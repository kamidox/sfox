#ifndef _FS_EML_MAIN_H_
#define _FS_EML_MAIN_H_

#define FS_EML_MIC_STR			16
#define FS_EML_MIN_STR			32
#define FS_EML_MID_STR			64
/* email rcpt max len */
#define FS_EML_RCPT_MAX_LEN		2048
/* email display name len */
#define FS_EML_NAME_LEN			32
/* email address name len */
#define FS_EML_ADDR_LEN			64
// email max text len
#define FS_EML_TEXT_MAX_LEN				(16 * 1024)	// 16K
// email max buf len, use for encode internal
#define FS_EML_TEXT_MAX_BUF				(FS_EML_TEXT_MAX_LEN * 4 / 3 + 100 )
// email header max len
#define FS_EML_HEAD_MAX_LEN				(16 * 1024)	// 16k
// email uid len, RFC1939 says UIDL consisting of one to 70 characters in the range 0x21 to 0x7E
#define FS_EML_UID_LEN					71

#endif
