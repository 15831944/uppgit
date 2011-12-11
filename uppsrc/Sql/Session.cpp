#include "Sql.h"

NAMESPACE_UPP

SqlSession::SqlSession()
{
	trace = NULL;
	traceslow = INT_MAX / 4;
	logerrors = false;
	usrlog = false;
	tracetime = false;
	dialect = 255;
	errorcode_number = Null;
	errorclass = Sql::ERROR_UNSPECIFIED;
	error_handler = NULL;
}

SqlSession::~SqlSession()
{
}

void           SqlSession::Begin()                                       { NEVER(); }
void           SqlSession::Commit()                                      { NEVER(); }
void           SqlSession::Rollback()                                    { NEVER(); }
String         SqlSession::Savepoint()                                   { NEVER(); return Null; }
void           SqlSession::RollbackTo(const String&)                     { NEVER(); }
bool           SqlSession::IsOpen() const                                { return false; }
int            SqlSession::GetTransactionLevel() const                   { return 0; }
RunScript      SqlSession::GetRunScript() const                          { return NULL; }
SqlConnection *SqlSession::CreateConnection()                            { return NULL; }
Vector<String> SqlSession::EnumUsers()                                   { return Vector<String>(); }
Vector<String> SqlSession::EnumDatabases()                               { return Vector<String>(); }
Vector<String> SqlSession::EnumTables(String database)                   { return Vector<String>(); }
Vector<String> SqlSession::EnumViews(String database)                    { return Vector<String>(); }
Vector<String> SqlSession::EnumSequences(String database)                { return Vector<String>(); }
Vector<String> SqlSession::EnumPrimaryKey(String database, String table) { return Vector<String>(); }
Vector<String> SqlSession::EnumReservedWords()                           { return Vector<String>(); }
String         SqlSession::EnumRowID(String database, String table)      { return Null; }

Vector<SqlColumnInfo> SqlSession::EnumColumns(String database, String table)
{
	Sql cursor(*this);
	Vector<SqlColumnInfo> info;
	SqlBool none;
	none.SetFalse();
	String full_name = database;
	if(!IsNull(database))
		full_name << '.';
	full_name << table;
	if(cursor.Execute(Select(SqlAll()).From(SqlSet(SqlId(full_name))).Where(none))) {
		info.SetCount(cursor.GetColumns());
		for(int i = 0; i < info.GetCount(); i++)
			info[i] = cursor.GetColumnInfo(i);
	}
	return info;
}

void   SqlSession::SetError(String error, String stmt, int code, const char *scode, Sql::ERRORCLASS clss) {
	if(error_handler && (*error_handler)(error, stmt, code, scode, clss))
		return;
	if(GetTransactionLevel() && errorstatement.GetCount())
		return;
	lasterror = error;
	errorstatement = stmt;
	errorcode_number = code;
	errorcode_string = scode;
	errorclass = clss;
	String err;
	err << "ERROR " << error << "(" << code << "): " << stmt << '\n';
	if(logerrors)
		BugLog() << err;
	if(GetTrace())
		*GetTrace() << err;
}

void SqlSession::InstallErrorHandler(bool (*handler)(String error, String stmt, int code, const char *scode, Sql::ERRORCLASS clss))
{
	error_handler = handler;
}

void SqlSession::SessionClose()
{
	if(sql) {
		sql->Cancel();
		sql.Clear();
	}
	if(sqlr) {
		sqlr->Cancel();
		sqlr.Clear();
	}
}

Sql& SqlSession::GetSessionSql()
{
	if(!sql)
		sql = new Sql(*this);
	return *sql;
}

Sql& SqlSession::GetSessionSqlR()
{
	if(!sqlr)
		sqlr = new Sql(*this);
	return *sqlr;
}

void   SqlSession::ClearError()
{
	lasterror.Clear();
	errorstatement.Clear();
	errorcode_number = Null;
	errorcode_string = Null;
	errorclass = Sql::ERROR_UNSPECIFIED;
}

StaticMutex          sDefs;
static SqlSession   *sGlobalSession;
static SqlSession   *sGlobalSessionR;
#ifdef _MULTITHREADED
thread__ SqlSession *sThreadSession;
thread__ SqlSession *sThreadSessionR;
#endif

void Sql::operator=(SqlSession& s)
{
	Mutex::Lock __(sDefs);
	if(this == &AppCursor()) {
	#ifdef _MULTITHREADED
		sThreadSession = &s;
		if(!sGlobalSession)
	#endif
			sGlobalSession = &s;
		return;
	}
	if(this == &AppCursorR()) {
	#ifdef _MULTITHREADED
		sThreadSessionR = &s;
		if(!sGlobalSessionR)
	#endif
			sGlobalSessionR = &s;
		return;
	}
	NEVER();
}

Sql& AppCursor()
{
#ifdef _MULTITHREADED
	if(sThreadSession)
		return sThreadSession->GetSessionSql();
#endif
	if(sGlobalSession)
		return sGlobalSession->GetSessionSql();
	static Sql *empty;
	ONCELOCK {
		static Sql h(Sql::NULLSQL);
		empty = &h;
	}
	return *empty;
}

Sql& AppCursorR()
{
#ifdef _MULTITHREADED
	if(sThreadSessionR)
		return sThreadSessionR->GetSessionSqlR();
#endif
	if(sGlobalSessionR)
		return sGlobalSessionR->GetSessionSqlR();
#ifdef _MULTITHREADED
	if(sThreadSession)
		return sThreadSession->GetSessionSqlR();
#endif
	if(sGlobalSession)
		return sGlobalSession->GetSessionSqlR();
	static Sql *empty;
	ONCELOCK {
		static Sql h(Sql::NULLSQL);
		empty = &h;
	}
	return *empty;
}

END_UPP_NAMESPACE