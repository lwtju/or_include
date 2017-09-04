#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<openssl/rsa.h>
#include<openssl/pem.h>
#include<openssl/err.h>
#include<time.h>

#include<luajit/lua.h>
#include<luajit/lauxlib.h>
#include<luajit/lualib.h>
 
#define PRIKEY "prikey.pem"
#define PUBKEY "pubkey.pem"
#define BUFFSIZE 4096
 
/************************************************************************
 * RSA加密解密函数
 *
 * file: test_rsa_encdec.c
 * gcc -Wall -shared -fPIC -o test_rsa.so test.c -lcrypto -lssl
 *
 ************************************************************************/
 
char* encrypt(const char *str, const char *pubkey_path)
{
    RSA *rsa = NULL;
    FILE *fp = NULL;
    char *en = NULL;
    int len = 0;
    int rsa_len = 0;
 
    if ((fp = fopen(pubkey_path, "r")) == NULL) {
        RSA_free(rsa);
        fclose(fp);
        return NULL;
    }
 
    /* 读取公钥PEM，PUBKEY格式PEM使用PEM_read_RSA_PUBKEY函数 */
    if ((rsa = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL)) == NULL) {
        RSA_free(rsa);
        fclose(fp);
        return NULL;
    }
 
    // RSA_print_fp(stdout, rsa, 0);
 
    len = strlen(str);
    rsa_len = RSA_size(rsa);
 
    en = (char *)malloc(rsa_len + 1);
    memset(en, 0, rsa_len + 1);
 
    if (RSA_public_encrypt(rsa_len, (unsigned char *)str, (unsigned char*)en, rsa, RSA_NO_PADDING) < 0) {
        RSA_free(rsa);
        fclose(fp);
        return NULL;
    }
 
    RSA_free(rsa);
    fclose(fp);
 
    return en;
}
 
char* decrypt(const char *str, const char *prikey_path)
{
    RSA *rsa = NULL;
    FILE *fp = NULL;
    char *de = NULL;
    int rsa_len = 0;

    if ((fp = fopen(prikey_path, "r")) == NULL) {
        RSA_free(rsa);
        fclose(fp);
        return NULL;
    }
 
    if ((rsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL)) == NULL) {
        RSA_free(rsa);
        fclose(fp);
        return NULL;
    }
 
    // RSA_print_fp(stdout, rsa, 0);
 
    rsa_len = RSA_size(rsa);
    de = (char *)malloc(rsa_len + 1);
    memset(de, 0, rsa_len + 1);
 
    if (RSA_private_decrypt(rsa_len, (unsigned char *)str, (unsigned char*)de, rsa, RSA_NO_PADDING) < 0) {
        RSA_free(rsa);
        fclose(fp);
		return NULL;
    }
 
    RSA_free(rsa);
    fclose(fp);
 
    return de;
}
