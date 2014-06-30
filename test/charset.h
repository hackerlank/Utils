/* 字符编码相关测试 */
/* 本文件的编码是GBK */

#ifndef TEST_CHARSET_H
#define TEST_CHARSET_H

#ifdef __cplusplus
extern "C" {
#endif

void	charset_detect();		/* 字符编码探测 */
void	charset_utf8();			/* UTF8相关测试 */
void	charset_file();			/* 文件编码相关 */
void	charset_unicode();		/* UNICODE字符转换 */
void	charset_wcs_mbcs();		/* 多字节宽字符转换 */
void	charset_wcs_utf8();		/* 宽字符串UTF-8转换 */
void	charset_mbcs_utf8();	/* 多字节UTF-8转换 */
void	charset_locale();		/* 系统本地字符集 */
void	charset_convert();		/* 字符编码转换 */

#ifdef __cplusplus
}
#endif

#endif