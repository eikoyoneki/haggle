/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Certificate.h"
#include "XMLMetadata.h"
#include "Trace.h"

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/opensslv.h>
#include <openssl/bn.h>

#include <string.h>
#include <base64.h>

Certificate::Certificate(X509 *_x) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_CERTIFICATE),
#endif
	stored(false), verified(false), hasSignature(true), x(_x), subject(""), issuer(""), 
	validity(""), pubKey(X509_get_pubkey(x)), x509_PEM_str(NULL)
{
	char buf[200];
	
	X509_NAME *subject_name = X509_get_subject_name(x);
	
	if (X509_NAME_get_text_by_NID(subject_name, NID_commonName, buf, 200))
		subject = buf;
	
	X509_NAME *issuer_name = X509_get_issuer_name(x);
	
	if (X509_NAME_get_text_by_NID(issuer_name, NID_commonName, buf, 200))
		issuer = buf;
	
	// TODO: set validity
}

Certificate::Certificate(const string& _subject, const string& _issuer, const string& _validity, const NodeId_t _owner, RSA *rsaPubKey) : 
#ifdef DEBUG_LEAKS
LeakMonitor(LEAK_TYPE_CERTIFICATE),
#endif
	stored(false), verified(false), hasSignature(false), x(NULL), subject(_subject), issuer(_issuer), 
		validity(_validity), pubKey(NULL), x509_PEM_str(NULL)
{
	memcpy(owner, _owner, sizeof(NodeId_t));
	
	x = X509_new();
	
	if (!x) {
		HAGGLE_ERR("Could not allocate X509 certificate struct\n");
#if HAVE_EXCEPTION
		throw MallocException(-1, "Could not allocate X509 certificate struct");
#else
		return;
#endif
	}
	
	X509_set_version(x, 2); 
	
	pubKey = EVP_PKEY_new();
	
	if (!pubKey) {
		X509_free(x);
		HAGGLE_ERR("Could not allocate X509 EVP_PKEY\n");
#if HAVE_EXCEPTION
		throw MallocException(-1, "Could not allocate X509 EVP_PKEY");
#else
		return;
#endif
	}
	
	EVP_PKEY_assign_RSA(pubKey, RSAPublicKey_dup(rsaPubKey));
	
	X509_set_pubkey(x, pubKey);

	/* Set validity.
	 FIXME: currently hardcoded
	 */
	int days = 30;
	X509_gmtime_adj(X509_get_notBefore(x),0);
	X509_gmtime_adj(X509_get_notAfter(x),(long)60*60*24*days);

	X509_NAME *subject_name = X509_get_subject_name(x);
	
	/* Set subject */
	//X509_NAME_add_entry_by_txt(subname,"C", MBSTRING_ASC, "SE", -1, -1, 0); 
	X509_NAME_add_entry_by_txt(subject_name, "CN", MBSTRING_ASC, (const unsigned char *)subject.c_str(), -1, -1, 0); 
	X509_NAME_add_entry_by_txt(subject_name, "O", MBSTRING_ASC, (const unsigned char *)"Haggle", -1, -1, 0);  
	
	X509_set_subject_name(x, subject_name); 

	/* Set issuer */
	X509_NAME *issuer_name = X509_get_issuer_name(x);
	
	X509_NAME_add_entry_by_txt(issuer_name, "CN", MBSTRING_ASC, (const unsigned char *)issuer.c_str(), -1, -1, 0); 
	X509_NAME_add_entry_by_txt(issuer_name, "O", MBSTRING_ASC, (const unsigned char *)"Haggle", -1, -1, 0);  
	
	X509_set_issuer_name(x, issuer_name); 
}

Certificate::~Certificate()
{
	if (pubKey)
		EVP_PKEY_free(pubKey);

	if (x)
		X509_free(x);
	
	if (x509_PEM_str)
		free(x509_PEM_str);
}

// Should somehow autodetect the OpenSSL capabilities/version. 
// One problem is MacOS X, because the headers say OpenSSL version 0.9.8j, but
// the library is 0.9.7
#if defined(OS_ANDROID)
// RSA_generate_key() is deprecated and removed in the Android OpenSSL version
#define HAVE_RSA_GENERATE_KEY_EX 1
#else
#define HAVE_RSA_GENERATE_KEY_EX 0
#endif

Certificate *Certificate::create(const string subject, const string issuer, const string validity, const NodeId_t owner, RSA **privKey)
{
        Certificate *c = NULL;
	RSA *pubKey, *keyPair;
	
#if HAVE_RSA_GENERATE_KEY_EX
        BIGNUM *e;

        e = BN_new();
        
        if (!e)
                return NULL;

        // The exponent is an odd number, typically 3, 17 or 65537.
        if (BN_set_word(e, 65537) == 0) {
                BN_free(e);
                return NULL;
        }

	keyPair = RSA_new();

        if (!keyPair) {
                BN_free(e);
                return NULL;
        }

	if (RSA_generate_key_ex(keyPair, RSA_KEY_LENGTH, e, NULL) == -1) {
		BN_free(e);
                goto out;
        }
	
        BN_free(e);
#else
	// RSA_generate_key is deprecated, but MacOS X seems to bundle an old version of OpenSSL
	// with only the old function.
	keyPair = RSA_generate_key(RSA_KEY_LENGTH, RSA_F4, NULL, NULL);
	
	if (!keyPair)
		return NULL;
#endif
	*privKey = RSAPrivateKey_dup(keyPair);
	
	if (!*privKey) 
                goto out;
	
	pubKey = RSAPublicKey_dup(keyPair);
	
	if (!pubKey) {
                RSA_free(*privKey);
                goto out;
        }
	
        c = new Certificate(subject, issuer, validity, owner, pubKey);

        if (!c) {
                RSA_free(pubKey);
                RSA_free(*privKey);
        }
out:           
	RSA_free(keyPair);
	
	return c;
}

Certificate *Certificate::create(const string subject, const string issuer, const string validity, const NodeId_t owner, const string pubKeyFile)
{
	FILE *f = fopen(pubKeyFile.c_str(), "r");

	if (!f) {
		HAGGLE_ERR("Could not open public key file %s\n", pubKeyFile.c_str());
		return NULL;
	}

	RSA *pubKey = PEM_read_RSAPublicKey(f, NULL, NULL, NULL);

	fclose(f);

	if (!pubKey) {
		HAGGLE_ERR("Could not read RSA public key\n");
		return NULL;
	}

	Certificate *c = new Certificate(subject, issuer, validity, owner, pubKey);

        if (!c) {
                RSA_free(pubKey);
                return NULL;
        }
        return c;
}

bool Certificate::createDigest(unsigned char digest[SHA_DIGEST_LENGTH], const string data) const
{
	SHA_CTX ctx;
        unsigned char *b, *bp;
        int len;
        
        len = i2d_PublicKey(pubKey, NULL);
        
        if (len < 0)
                return false;
        
        bp = b = new unsigned char[len+1];
        
        if (!b)
                return false;

        len = i2d_PublicKey(pubKey, &bp);
  
        if (len < 0) {
                delete [] b;
                return false;
        }

	SHA1_Init(&ctx);

	SHA1_Update(&ctx, data.c_str(), data.length());
	SHA1_Update(&ctx, subject.c_str(), subject.length());
	SHA1_Update(&ctx, issuer.c_str(), issuer.length());;
	SHA1_Update(&ctx, validity.c_str(), validity.length());
	SHA1_Update(&ctx, b, len);

	SHA1_Final(digest, &ctx);

        delete [] b;

	return true;
}

RSA *Certificate::getPubKey()
{
	return EVP_PKEY_get1_RSA(pubKey);
}

void Certificate::printPubKey() const
{
	char key_str[5000];
	BIO *bp = BIO_new(BIO_s_mem());
	
	if (!bp)
		return;

	RSA_print(bp, getPubKey(), 0);
	
	memset(key_str, '\0', sizeof(key_str));
	BIO_read(bp, key_str, sizeof(key_str));
	
	printf("pubkey: %s\n", key_str);
	
}

const RSA *Certificate::getPubKey() const
{
	return EVP_PKEY_get1_RSA(pubKey);
}

bool Certificate::isSigned() const
{
        return hasSignature;
}


bool Certificate::isOwner(const NodeId_t owner) const
{
	if (memcmp(owner, this->owner, sizeof(NodeId_t)) == 0)
		return true;
	
	return false;
}

bool Certificate::isSubject(const string subject) const
{
	return this->subject == subject;
}


bool Certificate::sign(RSA *key)
{
	bool res = false;
	
	EVP_PKEY *pkey = EVP_PKEY_new();
	
	if (!pkey) {
		HAGGLE_ERR("Could not allocate EVP_PKEY\n");
		return false;
	}
	
	EVP_PKEY_set1_RSA(pkey, key);
		
	if (X509_sign(x, pkey, EVP_sha1())) 
		hasSignature = res = true;
	
	EVP_PKEY_free(pkey);
	
	return res;
}

bool Certificate::verifySignature(RSA *key)
{
	bool res = false;
	
	if (verified)
		return true;
	
	EVP_PKEY *pkey = EVP_PKEY_new();
	
	if (!pkey) {
		HAGGLE_ERR("Could not allocate EVP_PKEY\n");
		return false;
	}
	
	EVP_PKEY_set1_RSA(pkey, key);
	
	if (X509_verify(x, pkey)) {
		verified = res = true;
	}
	
	EVP_PKEY_free(pkey);
	
	return res;
}

/**
	Convert certificate to a human readable string.
 */
string Certificate::toString() const
{
	char x509_str[10000];
	
	if (!x)
		return x509_str;
		
	BIO *bp = BIO_new(BIO_s_mem());
	
	if (!bp)
		return x509_str;
	
	memset(x509_str, '\0', sizeof(x509_str));
		
	if (X509_print(bp, x))
		BIO_read(bp, x509_str, sizeof(x509_str));
	
	BIO_free(bp);
	
	return x509_str;
}

static char *X509ToPEMAlloc(X509 *x)
{
	char *x509_str = NULL;
	int len  = 0;

	if (!x)
		return NULL;
	
	BIO *bp = BIO_new(BIO_s_mem());
	
	if (!bp)
		return NULL;
	
	if (!PEM_write_bio_X509(bp, x))
		goto done;
	
	/* Get the length of the data written */	
	len = BIO_ctrl_pending(bp);	
	
	if (len <= 0)
		goto done;
	
	/* Allocate enough memory to hold the PEM string */
	x509_str = (char *)malloc(len + 1);
	
	if (!x509_str)
		goto done;
	
	len = BIO_read(bp, x509_str, len);
	
	if (len <= 0) {
		free(x509_str);
		x509_str = NULL;
	}
	
	x509_str[len] = '\0';
	
done:
	BIO_free(bp);
	
	return x509_str;
}

const char *Certificate::toPEM() const
{
	if (!x509_PEM_str)	
		const_cast<Certificate *>(this)->x509_PEM_str = X509ToPEMAlloc(x);
	
	return x509_PEM_str;
}

Metadata *Certificate::toMetadata() const
{
        Metadata *m;

	if (!x)
		return NULL;
	
        m = new XMLMetadata("Certificate");
        
        if (!m)
                return NULL;

	if (!x509_PEM_str) {
		
		const_cast<Certificate *>(this)->x509_PEM_str = X509ToPEMAlloc(x);
	
		if (!x509_PEM_str) {
			delete m;
			return NULL;
		}
	}
	
	m->setContent(x509_PEM_str);
	
        return m;
}

Certificate *Certificate::fromPEM(const string pem)
{
	X509 *x = NULL;
	Certificate *c = NULL;

	BIO *bp = BIO_new(BIO_s_mem());
	
	if (!bp)
		return NULL;
	
	if (!BIO_puts(bp, pem.c_str()))
		goto done;
	
	x = PEM_read_bio_X509(bp, &x, NULL, NULL);
	
	if (x) {
		c = new Certificate(x);
	}
	
done:
	BIO_free(bp);
	
	return c;
}

Certificate *Certificate::fromMetadata(const Metadata& m)
{
	X509 *x = NULL;
	Certificate *c = NULL;
	
	if (m.getName() != "Certificate")
		return NULL;
	
	BIO *bp = BIO_new(BIO_s_mem());
	
	if (!bp)
		return NULL;
	
	if (!BIO_puts(bp, m.getContent().c_str()))
		goto done;
	
	x = PEM_read_bio_X509(bp, &x, NULL, NULL);
	
	if (x) {
		c = new Certificate(x);
	}
	
done:
	BIO_free(bp);
	
	return c;
}

bool operator==(const Certificate& c1, const Certificate& c2)
{
	return (X509_cmp(c1.x, c2.x) && c1.subject == c2.subject && c1.validity == c2.validity && c1.issuer == c2.issuer);
}

bool operator!=(const Certificate& c1, const Certificate& c2)
{
	return !(c1 == c2);
}