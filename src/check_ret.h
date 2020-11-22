#pragma once


#define CHECK_RET(X, V) do{ if(X){}else{ mk::check_ret_failed(__FILE__, __LINE__, #X); return V; } }while(false)
#define CHECK_RET_F(X) do{ if(X){}else{ mk::check_ret_failed(__FILE__, __LINE__, #X); return false; } }while(false)
#define CHECK_RET_V(X) do{ if(X){}else{ mk::check_ret_failed(__FILE__, __LINE__, #X); return; } }while(false)


namespace mk
{
	void check_ret_failed(char const* const file, int const line, char const* const expr);
}
