#include "DQL.h"

#include <QMap>
#include <QSet>
#include <QVector>
#include <QStringList>
#include <QRegularExpression>
#include <QtMath>
#include <algorithm>
#include <stdexcept>

namespace {

enum class AggType {
    None,
    CountAll,
    CountCol,
    CountDistinct,
    Sum,
    Avg,
    Min,
    Max,
    Stddev,
    Variance,
    GroupConcat
};

enum class TokenKind {
    Identifier,
    String,
    Number,
    LParen,
    RParen,
    Comma,
    Op,
    End
};

struct Token {
    TokenKind kind = TokenKind::End;
    QString text;
};

struct TableData {
    QString tableName;
    QString alias;
    DDL::Table schema;
    QVector<QVector<QString>> rows;
};

struct SelectItem {
    bool isStar = false;
    AggType agg = AggType::None;
    QString expr;
    QString alias;
    bool distinct = false;  // for COUNT(DISTINCT col)
};

struct JoinClause {
    bool leftJoin = false;
    QString tableName;
    QString alias;
    QString leftCol;
    QString rightCol;
};

struct OrderByItem {
    QString expr;
    bool ascending = true;
};

struct QuerySpec {
    bool distinct = false;
    QVector<SelectItem> selectItems;
    QString baseTable;
    QString baseAlias;
    QVector<JoinClause> joins;
    QString whereExpr;
    QVector<QString> groupByCols;
    QString havingExpr;
    QVector<OrderByItem> orderByItems;
    bool hasLimit = false;
    int limit = -1;
    int offset = 0;
};

struct GroupData {
    QVector<QMap<QString, QString>> rows;
};

class LexerLite {
public:
    explicit LexerLite(const QString &src) : m_src(src), m_pos(0) {}

    QVector<Token> scanAll() {
        QVector<Token> out;
        while (true) {
            Token t = next();
            out.append(t);
            if (t.kind == TokenKind::End) {
                break;
            }
        }
        return out;
    }

private:
    Token next() {
        skipSpaces();
        if (m_pos >= m_src.size()) return {TokenKind::End, ""};
        const QChar c = m_src[m_pos];

        if (c == '(') { m_pos++; return {TokenKind::LParen, "("}; }
        if (c == ')') { m_pos++; return {TokenKind::RParen, ")"}; }
        if (c == ',') { m_pos++; return {TokenKind::Comma, ","}; }

        if (c == '\'' ) {
            m_pos++;
            int start = m_pos;
            while (m_pos < m_src.size() && m_src[m_pos] != '\'') m_pos++;
            QString s = m_src.mid(start, m_pos - start);
            if (m_pos < m_src.size()) m_pos++;
            return {TokenKind::String, s};
        }

        if (c.isDigit() || (c == '-' && m_pos + 1 < m_src.size() && m_src[m_pos + 1].isDigit())) {
            int start = m_pos++;
            while (m_pos < m_src.size() && (m_src[m_pos].isDigit() || m_src[m_pos] == '.')) m_pos++;
            return {TokenKind::Number, m_src.mid(start, m_pos - start)};
        }

        if (c == '<' || c == '>' || c == '!' || c == '=') {
            if (m_pos + 1 < m_src.size()) {
                const QString two = m_src.mid(m_pos, 2);
                if (two == "<=" || two == ">=" || two == "!=" || two == "<>") {
                    m_pos += 2;
                    return {TokenKind::Op, two};
                }
            }
            m_pos++;
            return {TokenKind::Op, QString(c)};
        }

        if (c.isLetter() || c == '_' ) {
            int start = m_pos++;
            while (m_pos < m_src.size()) {
                const QChar x = m_src[m_pos];
                if (x.isLetterOrNumber() || x == '_' || x == '.') {
                    m_pos++;
                } else {
                    break;
                }
            }
            return {TokenKind::Identifier, m_src.mid(start, m_pos - start)};
        }

        m_pos++;
        return next();
    }

    void skipSpaces() {
        while (m_pos < m_src.size() && m_src[m_pos].isSpace()) m_pos++;
    }

    QString m_src;
    int m_pos;
};

static QString toUpperTrim(const QString &s) {
    return s.trimmed().toUpper();
}

static int findTopLevelKeyword(const QString &sql, const QString &kw, int start = 0) {
    int depth = 0;
    bool inStr = false;
    const QString upper = sql.toUpper();
    const QString key = kw.toUpper();

    for (int i = start; i < upper.size(); ++i) {
        const QChar c = upper[i];
        if (c == '\'') inStr = !inStr;
        if (inStr) continue;
        if (c == '(') depth++;
        else if (c == ')') depth--;
        if (depth != 0) continue;

        if (i + key.size() <= upper.size() && upper.mid(i, key.size()) == key) {
            const bool left = (i == 0) || !upper[i - 1].isLetterOrNumber();
            const bool right = (i + key.size() >= upper.size()) || !upper[i + key.size()].isLetterOrNumber();
            if (left && right) return i;
        }
    }
    return -1;
}

static QStringList splitTopLevel(const QString &s, QChar sep) {
    QStringList out;
    int depth = 0;
    bool inStr = false;
    int start = 0;
    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s[i];
        if (c == '\'') inStr = !inStr;
        if (inStr) continue;
        if (c == '(') depth++;
        else if (c == ')') depth--;
        else if (c == sep && depth == 0) {
            out.append(s.mid(start, i - start).trimmed());
            start = i + 1;
        }
    }
    out.append(s.mid(start).trimmed());
    return out;
}

static AggType parseAggName(const QString &name) {
    const QString n = name.toUpper();
    if (n == "COUNT") return AggType::CountCol;
    if (n == "SUM") return AggType::Sum;
    if (n == "AVG") return AggType::Avg;
    if (n == "MIN") return AggType::Min;
    if (n == "MAX") return AggType::Max;
    if (n == "STDDEV") return AggType::Stddev;
    if (n == "VARIANCE") return AggType::Variance;
    if (n == "GROUP_CONCAT") return AggType::GroupConcat;
    return AggType::None;
}

static bool isNumericAgg(AggType t) {
    return t == AggType::Sum || t == AggType::Avg || t == AggType::Stddev || t == AggType::Variance;
}

static QString aggDisplayName(const SelectItem &s) {
    switch (s.agg) {
    case AggType::CountAll: return "COUNT(*)";
    case AggType::CountCol: return "COUNT(" + s.expr + ")";
    case AggType::CountDistinct: return "COUNT(DISTINCT " + s.expr + ")";
    case AggType::Sum: return "SUM(" + s.expr + ")";
    case AggType::Avg: return "AVG(" + s.expr + ")";
    case AggType::Min: return "MIN(" + s.expr + ")";
    case AggType::Max: return "MAX(" + s.expr + ")";
    case AggType::Stddev: return "STDDEV(" + s.expr + ")";
    case AggType::Variance: return "VARIANCE(" + s.expr + ")";
    case AggType::GroupConcat: return "GROUP_CONCAT(" + s.expr + ")";
    default: return s.expr;
    }
}

static QString resolveColumn(const QMap<QString, QString> &row, const QVector<TableData> &tables, const QString &ident) {
    const QString direct = ident.toLower();
    if (row.contains(direct)) return row.value(direct);

    if (ident.contains('.')) {
        const QString key = ident.toLower();
        if (!row.contains(key)) {
            throw std::invalid_argument(QString("列不存在: %1").arg(ident).toStdString());
        }
        return row.value(key);
    }

    QString foundKey;
    int hit = 0;
    for (const TableData &t : tables) {
        const QString k1 = (t.alias + "." + ident).toLower();
        const QString k2 = (t.tableName + "." + ident).toLower();
        if (row.contains(k1)) { foundKey = k1; hit++; }
        else if (row.contains(k2)) { foundKey = k2; hit++; }
    }
    if (hit == 0) {
        throw std::invalid_argument(QString("列不存在: %1").arg(ident).toStdString());
    }
    if (hit > 1) {
        throw std::invalid_argument(QString("歧义列名: %1，请使用 表名.列名").arg(ident).toStdString());
    }
    return row.value(foundKey);
}

class BoolExpr {
public:
    BoolExpr(const QString &expr, const QVector<TableData> &tables)
        : m_tokens(LexerLite(expr).scanAll()), m_tables(tables), m_pos(0) {}

    bool eval(const QMap<QString, QString> &row) {
        m_row = &row;
        m_pos = 0;
        return parseOr();
    }

private:
    bool parseOr() {
        bool v = parseAnd();
        while (matchId("OR")) v = v || parseAnd();
        return v;
    }

    bool parseAnd() {
        bool v = parseNot();
        while (matchId("AND")) v = v && parseNot();
        return v;
    }

    bool parseNot() {
        if (matchId("NOT")) return !parseNot();
        return parsePrimary();
    }

    bool parsePrimary() {
        if (match(TokenKind::LParen)) {
            const bool v = parseOr();
            expect(TokenKind::RParen, ")");
            return v;
        }
        return parseCompare();
    }

    bool parseCompare() {
        const QString left = parseValue();

        // IS NULL / IS NOT NULL
        if (cur().kind == TokenKind::Identifier && cur().text.toUpper() == "IS") {
            m_pos++;
            bool negate = false;
            if (cur().kind == TokenKind::Identifier && cur().text.toUpper() == "NOT") {
                negate = true;
                m_pos++;
            }
            if (cur().kind != TokenKind::Identifier || cur().text.toUpper() != "NULL") {
                throw std::invalid_argument("IS 后期望 NULL");
            }
            m_pos++;
            const bool isNull = left.isEmpty();
            return negate ? !isNull : isNull;
        }

        // NOT IN / IN
        if (cur().kind == TokenKind::Identifier && cur().text.toUpper() == "NOT") {
            Token saved = cur();
            int savedPos = m_pos;
            m_pos++;
            if (cur().kind == TokenKind::Identifier && cur().text.toUpper() == "IN") {
                m_pos++;
                return !evalInList(left);
            }
            if (cur().kind == TokenKind::Identifier && cur().text.toUpper() == "LIKE") {
                m_pos++;
                return !evalLike(left);
            }
            if (cur().kind == TokenKind::Identifier && cur().text.toUpper() == "BETWEEN") {
                m_pos++;
                return !evalBetween(left);
            }
            // Not a recognized NOT combination, backtrack
            m_pos = savedPos;
        }

        if (cur().kind == TokenKind::Identifier && cur().text.toUpper() == "IN") {
            m_pos++;
            return evalInList(left);
        }

        if (cur().kind == TokenKind::Identifier && cur().text.toUpper() == "BETWEEN") {
            m_pos++;
            return evalBetween(left);
        }

        if (cur().kind == TokenKind::Identifier && cur().text.toUpper() == "LIKE") {
            m_pos++;
            return evalLike(left);
        }

        Token op = cur();
        if (op.kind != TokenKind::Op) {
            throw std::invalid_argument("条件表达式缺少比较运算符");
        }
        m_pos++;
        const QString right = parseValue();
        const QString o = op.text.toUpper();

        const bool lNull = left.isEmpty();
        const bool rNull = right.isEmpty();
        if (lNull || rNull) {
            if (o == "=") return lNull && rNull;
            if (o == "!=" || o == "<>") return lNull != rNull;
            return false;
        }

        bool ln = false, rn = false;
        const double ld = left.toDouble(&ln);
        const double rd = right.toDouble(&rn);
        if (ln && rn) {
            if (o == "=") return qFuzzyCompare(ld + 1.0, rd + 1.0);
            if (o == "!=" || o == "<>") return !qFuzzyCompare(ld + 1.0, rd + 1.0);
            if (o == "<") return ld < rd;
            if (o == "<=") return ld <= rd;
            if (o == ">") return ld > rd;
            if (o == ">=") return ld >= rd;
        } else {
            if (o == "=") return left == right;
            if (o == "!=" || o == "<>") return left != right;
            if (o == "<") return left < right;
            if (o == "<=") return left <= right;
            if (o == ">") return left > right;
            if (o == ">=") return left >= right;
        }
        throw std::invalid_argument(QString("不支持的比较操作: %1").arg(o).toStdString());
    }

    bool evalLike(const QString &left) {
        const QString pattern = parseValue();
        QString re;
        for (int i = 0; i < pattern.size(); ++i) {
            const QChar c = pattern[i];
            if (c == '%') re += ".*";
            else if (c == '_') re += ".";
            else re += QRegularExpression::escape(QString(c));
        }
        return QRegularExpression("^" + re + "$", QRegularExpression::CaseInsensitiveOption).match(left).hasMatch();
    }

    bool evalBetween(const QString &left) {
        const QString low = parseValue();
        if (!matchId("AND")) {
            throw std::invalid_argument("BETWEEN 需要 AND");
        }
        const QString high = parseValue();

        if (left.isEmpty()) return false;

        bool lok = false, hok = false, ok = false;
        const double ld = left.toDouble(&ok);
        const double lowd = low.toDouble(&lok);
        const double highd = high.toDouble(&hok);

        if (ok && lok && hok) {
            return ld >= lowd && ld <= highd;
        }
        // String comparison fallback
        return left >= low && left <= high;
    }

    bool evalInList(const QString &left) {
        expect(TokenKind::LParen, "(");
        QSet<QString> values;
        while (true) {
            QString v = parseValue();
            values.insert(v);
            if (match(TokenKind::Comma)) continue;
            break;
        }
        expect(TokenKind::RParen, ")");
        return values.contains(left);
    }

    QString parseValue() {
        Token t = cur();
        if (t.kind == TokenKind::String || t.kind == TokenKind::Number) { m_pos++; return t.text; }
        if (t.kind == TokenKind::Identifier) {
            m_pos++;
            if (t.text.toUpper() == "NULL") return "";
            return resolveColumn(*m_row, m_tables, t.text);
        }
        throw std::invalid_argument("条件表达式中的值非法");
    }

    bool match(TokenKind k) {
        if (cur().kind == k) { m_pos++; return true; }
        return false;
    }
    bool matchId(const QString &kw) {
        if (cur().kind == TokenKind::Identifier && cur().text.toUpper() == kw) { m_pos++; return true; }
        return false;
    }
    void expect(TokenKind k, const QString &msg) {
        if (!match(k)) throw std::invalid_argument(QString("语法错误，缺少 %1").arg(msg).toStdString());
    }
    Token cur() const { return (m_pos < m_tokens.size()) ? m_tokens[m_pos] : Token{TokenKind::End, ""}; }

    QVector<Token> m_tokens;
    const QVector<TableData> &m_tables;
    int m_pos;
    const QMap<QString, QString> *m_row = nullptr;
};

static DDL::Table loadSchemaByName(const DDL::DataBase &db, const QString &tableName) {
    const QString path = db.path + "/" + tableName + "/" + tableName + ".tbs";
    DDL::Table t = DDL::loadSchema(path);
    if (t.name.isEmpty()) {
        throw std::invalid_argument(QString("表不存在: %1").arg(tableName).toStdString());
    }
    return t;
}

static TableData loadTable(const DDL::DataBase &db, const QString &tableName, const QString &alias) {
    TableData td;
    td.tableName = tableName;
    td.alias = alias.isEmpty() ? tableName : alias;
    td.schema = loadSchemaByName(db, tableName);
    td.rows = DDL::loadTableData(td.schema, db.path);
    return td;
}

static QuerySpec parseQuery(const QString &sqlRaw) {
    QString sql = sqlRaw.trimmed();
    if (sql.endsWith(';')) sql.chop(1);
    const QString upper = sql.toUpper();
    if (!upper.startsWith("SELECT ")) {
        throw std::invalid_argument("仅支持 SELECT 查询");
    }

    const int fromPos = findTopLevelKeyword(sql, "FROM");
    if (fromPos < 0) throw std::invalid_argument("SELECT 缺少 FROM");

    QuerySpec q;
    QString selectPart = sql.mid(6, fromPos - 6).trimmed();
    QString tail = sql.mid(fromPos + 4).trimmed();

    // Handle DISTINCT
    if (selectPart.toUpper().startsWith("DISTINCT ")) {
        q.distinct = true;
        selectPart = selectPart.mid(9).trimmed();
    }

    const int wherePos = findTopLevelKeyword(tail, "WHERE");
    const int groupPos = findTopLevelKeyword(tail, "GROUP BY");
    const int havingPos = findTopLevelKeyword(tail, "HAVING");
    const int orderPos = findTopLevelKeyword(tail, "ORDER BY");
    const int limitPos = findTopLevelKeyword(tail, "LIMIT");
    int cut = tail.size();
    for (int p : {wherePos, groupPos, havingPos, orderPos, limitPos}) if (p >= 0) cut = qMin(cut, p);
    const QString fromJoinPart = tail.left(cut).trimmed();

    QString wherePart, groupPart, havingPart, orderPart, limitPart;
    if (wherePos >= 0) {
        int end = tail.size();
        for (int p : {groupPos, havingPos, orderPos, limitPos}) if (p > wherePos) end = qMin(end, p);
        wherePart = tail.mid(wherePos + 5, end - (wherePos + 5)).trimmed();
    }
    if (groupPos >= 0) {
        int end = tail.size();
        for (int p : {havingPos, orderPos, limitPos}) if (p > groupPos) end = qMin(end, p);
        groupPart = tail.mid(groupPos + 8, end - (groupPos + 8)).trimmed();
    }
    if (havingPos >= 0) {
        int end = tail.size();
        for (int p : {orderPos, limitPos}) if (p > havingPos) end = qMin(end, p);
        havingPart = tail.mid(havingPos + 6, end - (havingPos + 6)).trimmed();
    }
    if (orderPos >= 0) {
        int end = tail.size();
        for (int p : {limitPos}) if (p > orderPos) end = qMin(end, p);
        orderPart = tail.mid(orderPos + 8, end - (orderPos + 8)).trimmed();
    }
    if (limitPos >= 0) {
        limitPart = tail.mid(limitPos + 5).trimmed();
    }

    const QStringList sitems = splitTopLevel(selectPart, ',');
    for (QString item : sitems) {
        SelectItem si;
        QString alias;
        const QRegularExpression asRe("\\s+AS\\s+", QRegularExpression::CaseInsensitiveOption);
        const QStringList asSplit = item.split(asRe);
        if (asSplit.size() == 2) {
            item = asSplit[0].trimmed();
            alias = asSplit[1].trimmed();
        }
        if (item == "*") {
            si.isStar = true;
        } else {
            // Match: FUNC(DISTINCT col) or FUNC(col) or FUNC(*)
            const QRegularExpression aggRe("^([A-Za-z_][A-Za-z0-9_]*)\\s*\\((.*)\\)$");
            auto m = aggRe.match(item.trimmed());
            if (m.hasMatch()) {
                const QString fn = m.captured(1);
                QString arg = m.captured(2).trimmed();
                si.agg = parseAggName(fn);
                if (si.agg == AggType::None) {
                    throw std::invalid_argument(QString("不支持的聚合函数: %1").arg(fn).toStdString());
                }
                if (si.agg == AggType::CountCol && arg == "*") {
                    si.agg = AggType::CountAll;
                } else if (si.agg == AggType::CountCol) {
                    // Check for COUNT(DISTINCT col)
                    const QString argUpper = arg.toUpper();
                    if (argUpper.startsWith("DISTINCT ")) {
                        si.agg = AggType::CountDistinct;
                        si.distinct = true;
                        arg = arg.mid(9).trimmed();
                    }
                }
                si.expr = arg;
            } else {
                si.expr = item.trimmed();
            }
        }
        si.alias = alias;
        q.selectItems.append(si);
    }

    QString fromWork = fromJoinPart;
    const QStringList fromWords = fromWork.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (fromWords.isEmpty()) throw std::invalid_argument("FROM 缺少表名");
    q.baseTable = fromWords[0];
    // Check if second word is an alias (not a keyword like JOIN/WHERE etc.)
    if (fromWords.size() >= 2) {
        const QString w2 = fromWords[1].toUpper();
        if (w2 != "LEFT" && w2 != "INNER" && w2 != "JOIN" && w2 != "ON" &&
            w2 != "WHERE" && w2 != "GROUP" && w2 != "HAVING" && w2 != "ORDER" && w2 != "LIMIT") {
            q.baseAlias = fromWords[1];
        } else {
            q.baseAlias = q.baseTable;
        }
    } else {
        q.baseAlias = q.baseTable;
    }

    // Find where JOIN clauses start in the original string
    QString remain;
    {
        const int tableEnd = fromWork.indexOf(fromWords[0], 0);
        int afterTable = tableEnd + fromWords[0].size();
        // skip whitespace
        while (afterTable < fromWork.size() && fromWork[afterTable].isSpace()) afterTable++;
        if (q.baseAlias != q.baseTable) {
            // alias was specified, skip it too
            const int aliasStart = fromWork.indexOf(fromWords[1], afterTable);
            afterTable = aliasStart + fromWords[1].size();
        }
        remain = fromWork.mid(afterTable).trimmed();
    }
    while (!remain.isEmpty()) {
        JoinClause jc;
        QString u = remain.toUpper();
        if (u.startsWith("LEFT JOIN ")) {
            jc.leftJoin = true;
            remain = remain.mid(10).trimmed();
        } else if (u.startsWith("INNER JOIN ")) {
            remain = remain.mid(11).trimmed();
        } else if (u.startsWith("JOIN ")) {
            remain = remain.mid(5).trimmed();
        } else {
            break;
        }

        const int onPos = findTopLevelKeyword(remain, "ON");
        if (onPos < 0) throw std::invalid_argument("JOIN 缺少 ON 条件");
        QString left = remain.left(onPos).trimmed();
        QString onExprAndTail = remain.mid(onPos + 2).trimmed();

        const QStringList tableBits = left.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (tableBits.isEmpty()) throw std::invalid_argument("JOIN 缺少表名");
        jc.tableName = tableBits[0];
        jc.alias = (tableBits.size() >= 2) ? tableBits[1] : jc.tableName;

        int nextJoin = findTopLevelKeyword(onExprAndTail, "LEFT JOIN");
        if (nextJoin < 0) nextJoin = findTopLevelKeyword(onExprAndTail, "INNER JOIN");
        if (nextJoin < 0) nextJoin = findTopLevelKeyword(onExprAndTail, "JOIN");
        QString onExpr = (nextJoin < 0) ? onExprAndTail : onExprAndTail.left(nextJoin).trimmed();
        remain = (nextJoin < 0) ? "" : onExprAndTail.mid(nextJoin).trimmed();

        const int eq = onExpr.indexOf('=');
        if (eq < 0) throw std::invalid_argument("ON 仅支持等值连接: a.col = b.col");
        jc.leftCol = onExpr.left(eq).trimmed();
        jc.rightCol = onExpr.mid(eq + 1).trimmed();
        if (!jc.leftCol.contains('.') || !jc.rightCol.contains('.')) {
            throw std::invalid_argument("ON 两侧必须使用 表名.列名");
        }
        q.joins.append(jc);
    }

    q.whereExpr = wherePart;
    if (!groupPart.isEmpty()) q.groupByCols = splitTopLevel(groupPart, ',').toVector();
    q.havingExpr = havingPart;

    // Parse ORDER BY
    if (!orderPart.isEmpty()) {
        const QStringList orderItems = splitTopLevel(orderPart, ',');
        for (const QString &oi : orderItems) {
            OrderByItem obi;
            QString trimmed = oi.trimmed();
            const QString tu = trimmed.toUpper();
            if (tu.endsWith(" DESC")) {
                obi.ascending = false;
                trimmed = trimmed.left(trimmed.size() - 5).trimmed();
            } else if (tu.endsWith(" ASC")) {
                obi.ascending = true;
                trimmed = trimmed.left(trimmed.size() - 4).trimmed();
            }
            obi.expr = trimmed;
            q.orderByItems.append(obi);
        }
    }

    if (!limitPart.isEmpty()) {
        q.hasLimit = true;
        QRegularExpression re1("^([0-9]+)\\s*,\\s*([0-9]+)$");
        QRegularExpression re2("^([0-9]+)\\s+OFFSET\\s+([0-9]+)$", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression re3("^([0-9]+)$");
        auto m1 = re1.match(limitPart);
        auto m2 = re2.match(limitPart);
        auto m3 = re3.match(limitPart);
        if (m1.hasMatch()) {
            q.offset = m1.captured(1).toInt();
            q.limit = m1.captured(2).toInt();
        } else if (m2.hasMatch()) {
            q.limit = m2.captured(1).toInt();
            q.offset = m2.captured(2).toInt();
        } else if (m3.hasMatch()) {
            q.limit = m3.captured(1).toInt();
            q.offset = 0;
        } else {
            throw std::invalid_argument("LIMIT 语法无效");
        }
    }

    return q;
}

static QString aggregateValue(AggType agg, const QVector<QMap<QString, QString>> &rows, const QString &col,
                              const QVector<TableData> &tables, bool distinct = false) {
    if (agg == AggType::CountAll) return QString::number(rows.size());
    if (rows.isEmpty()) return "";

    QVector<QString> vals;
    vals.reserve(rows.size());
    for (const auto &r : rows) vals.append(resolveColumn(r, tables, col));

    if (agg == AggType::CountCol) {
        int c = 0;
        for (const QString &v : vals) if (!v.isEmpty()) c++;
        return QString::number(c);
    }
    if (agg == AggType::CountDistinct) {
        QSet<QString> seen;
        for (const QString &v : vals) if (!v.isEmpty()) seen.insert(v);
        return QString::number(seen.size());
    }
    if (agg == AggType::Min) {
        QString best;
        bool init = false;
        for (const QString &v : vals) if (!v.isEmpty()) { if (!init || v < best) {best = v; init = true;} }
        return best;
    }
    if (agg == AggType::Max) {
        QString best;
        bool init = false;
        for (const QString &v : vals) if (!v.isEmpty()) { if (!init || v > best) {best = v; init = true;} }
        return best;
    }
    if (agg == AggType::GroupConcat) {
        QStringList parts;
        for (const QString &v : vals) if (!v.isEmpty()) parts.append(v);
        return parts.join(",");
    }

    QVector<double> ns;
    for (const QString &v : vals) {
        if (v.isEmpty()) continue;
        bool ok = false;
        const double d = v.toDouble(&ok);
        if (!ok) throw std::invalid_argument(QString("聚合函数需要数值列: %1").arg(col).toStdString());
        ns.append(d);
    }
    if (ns.isEmpty()) return "";
    double sum = 0.0;
    for (double d : ns) sum += d;
    if (agg == AggType::Sum) return QString::number(sum);
    if (agg == AggType::Avg) return QString::number(sum / ns.size());
    double mean = sum / ns.size();
    double var = 0.0;
    for (double d : ns) { const double x = d - mean; var += x * x; }
    var /= ns.size();
    if (agg == AggType::Variance) return QString::number(var);
    if (agg == AggType::Stddev) return QString::number(qSqrt(var));
    return "";
}

static QString normalizeHavingExpr(const QString &expr, const QVector<SelectItem> &items) {
    QString out = expr;
    // Replace from longest to shortest to avoid partial matches
    // Build list of (function_string, placeholder) pairs
    QVector<QPair<QString, QString>> replacements;
    int idx = 0;
    for (const SelectItem &i : items) {
        if (i.agg == AggType::None) continue;
        QString fn;
        switch (i.agg) {
        case AggType::CountAll: fn = "COUNT(*)"; break;
        case AggType::CountCol: fn = "COUNT(" + i.expr + ")"; break;
        case AggType::CountDistinct: fn = "COUNT(DISTINCT " + i.expr + ")"; break;
        case AggType::Sum: fn = "SUM(" + i.expr + ")"; break;
        case AggType::Avg: fn = "AVG(" + i.expr + ")"; break;
        case AggType::Min: fn = "MIN(" + i.expr + ")"; break;
        case AggType::Max: fn = "MAX(" + i.expr + ")"; break;
        case AggType::Stddev: fn = "STDDEV(" + i.expr + ")"; break;
        case AggType::Variance: fn = "VARIANCE(" + i.expr + ")"; break;
        case AggType::GroupConcat: fn = "GROUP_CONCAT(" + i.expr + ")"; break;
        default: break;
        }
        const QString ph = QString("__agg%1").arg(idx++);
        replacements.append(qMakePair(fn, ph));
    }
    // Sort by function string length descending to avoid partial matches
    std::sort(replacements.begin(), replacements.end(),
              [](const auto &a, const auto &b) { return a.first.size() > b.first.size(); });
    for (const auto &r : replacements) {
        out.replace(QRegularExpression(QRegularExpression::escape(r.first), QRegularExpression::CaseInsensitiveOption), r.second);
    }
    return out;
}

static QString renderTable(const QStringList &headers, const QVector<QStringList> &rows) {
    // Calculate column widths
    QVector<int> widths;
    for (const QString &h : headers) widths.append(h.size());
    for (const QStringList &row : rows) {
        for (int i = 0; i < row.size() && i < headers.size(); ++i) {
            widths[i] = qMax(widths[i], row[i].size());
        }
    }

    // Header
    QString out;
    for (int i = 0; i < headers.size(); ++i) {
        if (i > 0) out += " | ";
        out += headers[i].leftJustified(widths[i]);
    }
    out += "\n";

    // Separator
    for (int i = 0; i < headers.size(); ++i) {
        if (i > 0) out += "-+-";
        out += QString(widths[i], '-');
    }
    out += "\n";

    // Rows
    if (rows.isEmpty()) {
        out += "(empty)";
        return out;
    }
    for (int i = 0; i < rows.size(); ++i) {
        for (int j = 0; j < rows[i].size() && j < headers.size(); ++j) {
            if (j > 0) out += " | ";
            out += rows[i][j].leftJustified(widths[j]);
        }
        if (i + 1 < rows.size()) out += "\n";
    }
    return out;
}

} // namespace

QString DQL::executeQuery(const DDL::DataBase &db, const QString &sql)
{
    if (db.path.isEmpty()) throw std::invalid_argument("未指定数据库");

    QuerySpec q = parseQuery(sql);

    QVector<TableData> tables;
    tables.append(loadTable(db, q.baseTable, q.baseAlias));
    for (const JoinClause &j : q.joins) tables.append(loadTable(db, j.tableName, j.alias));

    QVector<QMap<QString, QString>> dataset;
    {
        const TableData &base = tables[0];
        for (const auto &r : base.rows) {
            QMap<QString, QString> row;
            for (int i = 0; i < base.schema.fields.size(); ++i) {
                const QString col = base.schema.fields[i].field_name;
                row[(base.alias + "." + col).toLower()] = r.value(i);
                row[(base.tableName + "." + col).toLower()] = r.value(i);
            }
            dataset.append(row);
        }
    }

    for (int ji = 0; ji < q.joins.size(); ++ji) {
        const JoinClause &j = q.joins[ji];
        const TableData &rt = tables[ji + 1];
        QVector<QMap<QString, QString>> joined;
        for (const auto &lrow : dataset) {
            bool hit = false;
            for (const auto &rr : rt.rows) {
                QMap<QString, QString> row = lrow;
                for (int c = 0; c < rt.schema.fields.size(); ++c) {
                    const QString col = rt.schema.fields[c].field_name;
                    row[(rt.alias + "." + col).toLower()] = rr.value(c);
                    row[(rt.tableName + "." + col).toLower()] = rr.value(c);
                }
                const QString lv = resolveColumn(row, tables, j.leftCol);
                const QString rv = resolveColumn(row, tables, j.rightCol);
                if (lv == rv) {
                    joined.append(row);
                    hit = true;
                }
            }
            if (j.leftJoin && !hit) {
                QMap<QString, QString> row = lrow;
                for (const auto &f : rt.schema.fields) {
                    row[(rt.alias + "." + f.field_name).toLower()] = "";
                    row[(rt.tableName + "." + f.field_name).toLower()] = "";
                }
                joined.append(row);
            }
        }
        dataset = joined;
    }

    if (!q.whereExpr.isEmpty()) {
        BoolExpr where(q.whereExpr, tables);
        QVector<QMap<QString, QString>> filtered;
        for (const auto &r : dataset) if (where.eval(r)) filtered.append(r);
        dataset = filtered;
    }

    bool hasAgg = false;
    for (const auto &s : q.selectItems) if (s.agg != AggType::None) hasAgg = true;
    const bool hasGroup = !q.groupByCols.isEmpty();
    if (!hasGroup && hasAgg) {
        for (const auto &s : q.selectItems) {
            if (!s.isStar && s.agg == AggType::None) {
                throw std::invalid_argument("存在聚合函数时，非聚合列必须在 GROUP BY 中");
            }
        }
    }
    if (hasGroup) {
        for (const auto &s : q.selectItems) {
            if (!s.isStar && s.agg == AggType::None) {
                bool inGroup = false;
                for (const QString &g : q.groupByCols) if (toUpperTrim(g) == toUpperTrim(s.expr)) inGroup = true;
                if (!inGroup) throw std::invalid_argument("非聚合列必须出现在 GROUP BY 中");
            }
        }
    }

    QVector<QStringList> outRows;
    QStringList headers;

    if (!hasGroup && !hasAgg) {
        bool selectAll = q.selectItems.size() == 1 && q.selectItems[0].isStar;
        QVector<SelectItem> cols = q.selectItems;
        if (selectAll) {
            cols.clear();
            for (const TableData &t : tables) {
                for (const auto &f : t.schema.fields) {
                    SelectItem si;
                    si.expr = t.alias + "." + f.field_name;
                    cols.append(si);
                }
            }
        }
        for (const auto &c : cols) headers.append(c.alias.isEmpty() ? (c.agg != AggType::None ? aggDisplayName(c) : c.expr) : c.alias);
        for (const auto &r : dataset) {
            QStringList line;
            for (const auto &c : cols) line.append(resolveColumn(r, tables, c.expr));
            outRows.append(line);
        }
    } else {
        QMap<QString, GroupData> groups;
        if (hasGroup) {
            for (const auto &r : dataset) {
                QStringList keyParts;
                for (const QString &g : q.groupByCols) keyParts.append(resolveColumn(r, tables, g));
                const QString key = keyParts.join('\x1f');
                groups[key].rows.append(r);
            }
        } else {
            groups["__all__"].rows = dataset;
        }

        QString havingExpr = normalizeHavingExpr(q.havingExpr, q.selectItems);
        for (auto it = groups.begin(); it != groups.end(); ++it) {
            const auto &rows = it.value().rows;
            QMap<QString, QString> one;
            if (!rows.isEmpty()) one = rows[0];

            QStringList line;
            int aggIdx = 0;
            QMap<QString, QString> havingRow = one;
            for (const SelectItem &s : q.selectItems) {
                QString value;
                if (s.isStar) {
                    throw std::invalid_argument("GROUP BY 查询不支持 SELECT *");
                } else if (s.agg == AggType::None) {
                    value = rows.isEmpty() ? "" : resolveColumn(rows[0], tables, s.expr);
                } else {
                    value = aggregateValue(s.agg, rows, s.expr, tables, s.distinct);
                    havingRow[QString("__agg%1").arg(aggIdx++)] = value;
                }
                if (headers.size() < q.selectItems.size()) headers.append(s.alias.isEmpty() ? (s.agg != AggType::None ? aggDisplayName(s) : s.expr) : s.alias);
                line.append(value);
            }

            if (!havingExpr.isEmpty()) {
                QVector<TableData> noTables;
                BoolExpr hv(havingExpr, noTables);
                if (!hv.eval(havingRow)) continue;
            }
            outRows.append(line);
        }
    }

    // ORDER BY
    if (!q.orderByItems.isEmpty()) {
        // Build a mapping from header name to column index
        // For ORDER BY we need to resolve expressions
        // We sort outRows based on the ORDER BY expressions
        // For simple cases (column names or aliases), use header index
        // For complex expressions, we need the dataset

        // First, try to resolve each ORDER BY item to a column index
        struct SortKey {
            int colIdx;
            bool ascending;
        };
        QVector<SortKey> sortKeys;

        for (const OrderByItem &obi : q.orderByItems) {
            SortKey sk;
            sk.ascending = obi.ascending;
            sk.colIdx = -1;

            // Try matching by header name
            for (int i = 0; i < headers.size(); ++i) {
                if (headers[i].compare(obi.expr, Qt::CaseInsensitive) == 0) {
                    sk.colIdx = i;
                    break;
                }
            }

            // Try resolving column expression (e.g., table.col)
            if (sk.colIdx < 0 && !dataset.isEmpty()) {
                for (int i = 0; i < headers.size(); ++i) {
                    // Check if this header corresponds to the ORDER BY expression
                    // headers[i] may be like "alias.col" or just "col"
                    QString headerLower = headers[i].toLower();
                    QString exprLower = obi.expr.toLower();
                    if (headerLower == exprLower) {
                        sk.colIdx = i;
                        break;
                    }
                    // Try without table prefix
                    if (exprLower.contains('.')) {
                        QString exprCol = exprLower.mid(exprLower.lastIndexOf('.') + 1);
                        if (headerLower == exprCol || headerLower.endsWith('.' + exprCol)) {
                            sk.colIdx = i;
                            break;
                        }
                    }
                    if (headerLower.contains('.')) {
                        QString headerCol = headerLower.mid(headerLower.lastIndexOf('.') + 1);
                        if (exprLower == headerCol || exprLower.endsWith('.' + headerCol)) {
                            sk.colIdx = i;
                            break;
                        }
                    }
                }
            }

            if (sk.colIdx < 0) {
                throw std::invalid_argument(QString("ORDER BY 列不存在: %1").arg(obi.expr).toStdString());
            }
            sortKeys.append(sk);
        }

        // Perform stable sort
        std::stable_sort(outRows.begin(), outRows.end(),
            [&sortKeys](const QStringList &a, const QStringList &b) {
                for (const SortKey &sk : sortKeys) {
                    const QString va = (sk.colIdx < a.size()) ? a[sk.colIdx] : "";
                    const QString vb = (sk.colIdx < b.size()) ? b[sk.colIdx] : "";

                    // Try numeric comparison
                    bool okA = false, okB = false;
                    const double da = va.toDouble(&okA);
                    const double db = vb.toDouble(&okB);

                    if (va.isEmpty() && !vb.isEmpty()) return !sk.ascending;
                    if (!va.isEmpty() && vb.isEmpty()) return sk.ascending;
                    if (va.isEmpty() && vb.isEmpty()) continue;

                    int cmp = 0;
                    if (okA && okB) {
                        if (da < db) cmp = -1;
                        else if (da > db) cmp = 1;
                    } else {
                        if (va < vb) cmp = -1;
                        else if (va > vb) cmp = 1;
                    }

                    if (cmp != 0) return sk.ascending ? (cmp < 0) : (cmp > 0);
                }
                return false;
            });
    }

    // DISTINCT
    if (q.distinct) {
        QSet<QString> seen;
        QVector<QStringList> deduped;
        for (const QStringList &row : outRows) {
            const QString key = row.join("\x1f");
            if (!seen.contains(key)) {
                seen.insert(key);
                deduped.append(row);
            }
        }
        outRows = deduped;
    }

    // LIMIT / OFFSET
    if (q.hasLimit) {
        QVector<QStringList> limited;
        for (int i = q.offset; i < outRows.size() && limited.size() < q.limit; ++i) limited.append(outRows[i]);
        outRows = limited;
    }

    return renderTable(headers, outRows);
}
