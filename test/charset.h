/* �ַ�������ز��� */
/* ���ļ��ı�����GBK */

#ifndef TEST_CHARSET_H
#define TEST_CHARSET_H

#ifdef __cplusplus
extern "C" {
#endif

void	charset_detect();		/* �ַ�����̽�� */
void	charset_utf8();			/* UTF8��ز��� */
void	charset_file();			/* �ļ�������� */
void	charset_unicode();		/* UNICODE�ַ�ת�� */
void	charset_wcs_mbcs();		/* ���ֽڿ��ַ�ת�� */
void	charset_wcs_utf8();		/* ���ַ���UTF-8ת�� */
void	charset_mbcs_utf8();	/* ���ֽ�UTF-8ת�� */
void	charset_locale();		/* ϵͳ�����ַ��� */
void	charset_convert();		/* �ַ�����ת�� */

#ifdef __cplusplus
}
#endif

#endif