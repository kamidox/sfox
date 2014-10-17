#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/web/FS_Cookie.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_Util.h" 
#include "inc/util/FS_List.h" 
#include "inc/util/FS_MemDebug.h"

FS_List GFS_Cookies = { &GFS_Cookies, &GFS_Cookies };

typedef struct FS_Cookie_Tag
{
	FS_List			list;

	FS_SINT4		age;
	FS_CHAR *		name;
	FS_CHAR *		value;
	FS_CHAR *		version;
	FS_CHAR *		domain;
	FS_CHAR *		path;
}FS_Cookie;

static FS_CHAR *FS_CookieName( FS_CHAR **cookies )
{
	FS_CHAR *cstr = *cookies, *name;
	FS_SINT4 len;
	
	while( *cstr != 0 
		&& ( *cstr == ' ' || *cstr == '\r' || *cstr == '\n' 
		|| *cstr == '\t' || *cstr == ',' || *cstr == ';' ) )
		cstr ++;

	name = cstr;
	while( *cstr && *cstr != '=' )
		cstr ++;

	if( *cstr == '=' )
	{
		len = cstr - name;
		name = FS_Strndup( name, len );
		cstr ++;	/* skip '=' char */
	}
	else
	{
		name = FS_NULL;
	}
	
	*cookies = cstr;
	return name;
}

static FS_CHAR *FS_CookieValue( FS_CHAR **cookies )
{
	FS_CHAR *cstr = *cookies, *value = FS_NULL, quote = 0;
	FS_SINT4 len;
	
	while( *cstr != 0 && ( *cstr == ' ' || *cstr == '\r' || *cstr == '\n' || *cstr == '\t' ) )
		cstr ++;

	if( *cstr != 0 )
	{
		value = cstr;
		if( *cstr == '"' || *cstr == '\'' )
		{
			quote = *cstr;
			cstr ++;
			value ++;
		}
		
		if( quote )
		{
			while( *cstr && *cstr != quote )
				cstr ++;
		}
		else
		{
			while( *cstr && *cstr != ' ' && *cstr != ',' && *cstr != ';' 
				&& *cstr != '\r' && *cstr != '\n' && *cstr != '\t'  )
				cstr ++;
		}
		
		len = cstr - value;
		value = FS_Strndup( value, len );
		cstr ++;	/* skip quote char */
	}
	
	*cookies = cstr;
	return value;
}

static FS_Cookie *FS_CookieMatch( FS_CHAR *url, FS_CHAR *name )
{
	FS_List *node;
	FS_Cookie *cookie;

	node = GFS_Cookies.next;
	while( node != &GFS_Cookies )
	{
		cookie = FS_ListEntry( node, FS_Cookie, list );
		node = node->next;

		if( cookie->domain && IFS_Strstr( url, cookie->domain )
			&& cookie->path && IFS_Strstr( url, cookie->path ) 
			&& cookie->name && IFS_Stricmp( name, cookie->name ) == 0 )
		{
			/* math cookie */
			return cookie;
		}
	}

	return FS_NULL;
}

static void FS_CookieFree( FS_Cookie *cookie )
{
	FS_SAFE_FREE( cookie->domain );
	FS_SAFE_FREE( cookie->path );
	FS_SAFE_FREE( cookie->name );
	FS_SAFE_FREE( cookie->value );
	FS_SAFE_FREE( cookie->version );

	IFS_Free( cookie );
}


void FS_CookieSave( FS_CHAR *url, FS_CHAR *cookies )
{
	FS_CHAR *name, *value;
	FS_Cookie *cookie = FS_NULL;

	while( *cookies != 0 )
	{
		name = FS_CookieName( &cookies );
		value = FS_CookieValue( &cookies );

		if( name == FS_NULL )
		{
			if( cookie && cookie->name )
			{
				/* a pending cookies. we add it to cookies list */
				if( cookie->domain == FS_NULL )
					cookie->domain = FS_UrlGetHost( url );
				if( cookie->path == FS_NULL )
					cookie->path = FS_UrlGetDir( url );
				
				FS_ListAddTail( &GFS_Cookies, &cookie->list );
			}
			break;
		}
		
		if( cookie && IFS_Stricmp( name, "Version" ) == 0 )
		{
			if( cookie->version ) IFS_Free( cookie->version );
			cookie->version = value;
			IFS_Free( name );
		}
		else if( cookie && IFS_Stricmp( name, "Domain" ) == 0 )
		{
			if( cookie->domain ) IFS_Free( cookie->domain );
			cookie->domain = value;
			IFS_Free( name );
		}
		else if( cookie && IFS_Stricmp( name, "Path" ) == 0 )
		{
			if( cookie->path ) IFS_Free( cookie->path );
			cookie->path = value;
			IFS_Free( name );
		}
		else if( cookie && IFS_Stricmp( name, "Max-Age" ) == 0 )
		{
			cookie->age = IFS_Atoi( value );
			IFS_Free( value );
			IFS_Free( name );
		}
		else if( cookie && (IFS_Stricmp( name, "Comment" ) == 0 || IFS_Stricmp( name, "Secure" ) == 0 || IFS_Stricmp( name, "Expires" ) == 0) )
		{
			/* we ignore these cookie attributes */
			FS_SAFE_FREE( value );
			IFS_Free( name );
		}
		else
		{
			/* here, we encounter a cookie name value pairs */
			if( cookie && cookie->name )
			{
				/* a pending cookies. we add it to cookies list */
				if( cookie->domain == FS_NULL )
					cookie->domain = FS_UrlGetHost( url );
				if( cookie->path == FS_NULL )
					cookie->path = FS_UrlGetDir( url );
				
				FS_ListAddTail( &GFS_Cookies, &cookie->list );
				cookie = FS_NULL;
			}

			if( cookie == FS_NULL )
			{
				cookie = FS_CookieMatch( url, name );
				if( cookie == FS_NULL )
				{
					cookie = FS_NEW( FS_Cookie );
					if( cookie )
						IFS_Memset( cookie, 0, sizeof(FS_Cookie) );
					else
						break;
				}
				else
				{
					FS_ListDel( &cookie->list );
				}
			}

			if( cookie->name ) IFS_Free( cookie->name );
			cookie->name = name;
			if( cookie->value ) IFS_Free( cookie->value );
			cookie->value = value;
		}
	}
}

void FS_CookieClear( void )
{
	FS_List *node;
	FS_Cookie *cookie;

	node = GFS_Cookies.next;
	while( node != &GFS_Cookies )
	{
		cookie = FS_ListEntry( node, FS_Cookie, list );
		node = node->next;
		
		FS_ListDel( &cookie->list );
		FS_CookieFree( cookie );
	}
}

FS_SINT4 FS_CookieGet( FS_CHAR *out, FS_SINT4 olen, FS_CHAR *url )
{
	FS_List *node;
	FS_Cookie *cookie;
	FS_SINT4 offset = 0, tlen;

	node = GFS_Cookies.prev;
	while( node != &GFS_Cookies )
	{
		cookie = FS_ListEntry( node, FS_Cookie, list );
		node = node->prev;
		
		if( cookie->domain && IFS_Strstr( url, cookie->domain )
			&& cookie->path && IFS_Strstr( url, cookie->path ) )
		{
			/* match cookie */
			if( offset == 0 )
			{
				IFS_Strcpy( out, "Cookie: " );
				offset = IFS_Strlen( out );
			}
			
			if( cookie->value )
			{
				tlen = IFS_Strlen( cookie->name ) + IFS_Strlen( cookie->value ) + 5;
				if( offset + tlen < olen )
				{
					if( IFS_Strchr( cookie->value, ' ' ) )
						IFS_Sprintf( out + offset, "%s=\"%s\"; ", cookie->name, cookie->value );
					else
						IFS_Sprintf( out + offset, "%s=%s; ", cookie->name, cookie->value );
					offset += IFS_Strlen( out + offset );
				}
			}
			else
			{
				tlen = IFS_Strlen( cookie->name ) + 5;
				if( offset + tlen + 1 < olen )
				{
					IFS_Sprintf( out + offset, "%s=; ", cookie->name );
					offset += IFS_Strlen( out + offset );
				}
			}
		}
	}

	if( offset >= 2 )
	{
		out[offset - 2] = '\r';
		out[offset - 1] = '\n';
	}

	return offset;
}

FS_SINT4 FS_CookieGetSize( void )
{
	return 0;
}

#endif	//FS_MODULE_WEB

