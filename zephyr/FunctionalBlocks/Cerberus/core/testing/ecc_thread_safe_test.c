// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "testing.h"
#include "crypto/ecc_thread_safe.h"
#include "mock/ecc_mock.h"
#include "testing/ecc_testing.h"
#include "testing/signature_testing.h"


static const char *SUITE = "ecc_thread_safe";


/*******************
 * Test cases
 *******************/

static void ecc_thread_safe_test_init (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	CuAssertPtrNotNull (test, engine.base.init_key_pair);
	CuAssertPtrNotNull (test, engine.base.init_public_key);
	CuAssertPtrNotNull (test, engine.base.generate_derived_key_pair);
	CuAssertPtrNotNull (test, engine.base.generate_key_pair);
	CuAssertPtrNotNull (test, engine.base.release_key_pair);
	CuAssertPtrNotNull (test, engine.base.get_signature_max_length);
	CuAssertPtrNotNull (test, engine.base.get_private_key_der);
	CuAssertPtrNotNull (test, engine.base.get_public_key_der);
	CuAssertPtrNotNull (test, engine.base.sign);
	CuAssertPtrNotNull (test, engine.base.verify);
	CuAssertPtrNotNull (test, engine.base.get_shared_secret_max_length);
	CuAssertPtrNotNull (test, engine.base.compute_shared_secret);

	status = ecc_mock_validate_and_release (&mock);
	CuAssertIntEquals (test, 0, status);

	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_init_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (NULL, &mock.base);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = ecc_thread_safe_init (&engine, NULL);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = ecc_mock_validate_and_release (&mock);
	CuAssertIntEquals (test, 0, status);
}

static void ecc_thread_safe_test_release_null (CuTest *test)
{
	TEST_START;

	ecc_thread_safe_release (NULL);
}

static void ecc_thread_safe_test_init_key_pair (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.init_key_pair, &mock, 0, MOCK_ARG (ECC_PRIVKEY_DER),
		MOCK_ARG (ECC_PRIVKEY_DER_LEN), MOCK_ARG (&priv_key), MOCK_ARG (&pub_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER,
		ECC_PRIVKEY_DER_LEN, &priv_key, &pub_key);
	CuAssertIntEquals (test, 0, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_init_key_pair_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.init_key_pair, &mock, ECC_ENGINE_KEY_PAIR_FAILED,
		MOCK_ARG (ECC_PRIVKEY_DER), MOCK_ARG (ECC_PRIVKEY_DER_LEN), MOCK_ARG (&priv_key),
		MOCK_ARG (&pub_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER,
		ECC_PRIVKEY_DER_LEN, &priv_key, &pub_key);
	CuAssertIntEquals (test, ECC_ENGINE_KEY_PAIR_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_init_key_pair_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.init_key_pair (NULL, (const uint8_t*) ECC_PRIVKEY_DER,
		ECC_PRIVKEY_DER_LEN, &priv_key, &pub_key);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_init_public_key (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.init_public_key, &mock, 0,
		MOCK_ARG (ECC_PUBKEY_DER), MOCK_ARG (ECC_PUBKEY_DER_LEN), MOCK_ARG (&pub_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.init_public_key (&engine.base, (const uint8_t*) ECC_PUBKEY_DER,
		ECC_PUBKEY_DER_LEN, &pub_key);
	CuAssertIntEquals (test, 0, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_init_public_key_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.init_public_key, &mock,
		ECC_ENGINE_PUBLIC_KEY_FAILED, MOCK_ARG (ECC_PUBKEY_DER), MOCK_ARG (ECC_PUBKEY_DER_LEN),
		MOCK_ARG (&pub_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.init_public_key (&engine.base, (const uint8_t*) ECC_PUBKEY_DER,
		ECC_PUBKEY_DER_LEN, &pub_key);
	CuAssertIntEquals (test, ECC_ENGINE_PUBLIC_KEY_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_init_public_key_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.init_public_key (NULL, (const uint8_t*) ECC_PUBKEY_DER,
		ECC_PUBKEY_DER_LEN, &pub_key);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_generate_derived_key_pair (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.generate_derived_key_pair, &mock, 0,
		MOCK_ARG (ECC_PRIVKEY), MOCK_ARG (ECC_PRIVKEY_LEN), MOCK_ARG (&priv_key),
		MOCK_ARG (&pub_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.generate_derived_key_pair (&engine.base, ECC_PRIVKEY, ECC_PRIVKEY_LEN,
		&priv_key, &pub_key);
	CuAssertIntEquals (test, 0, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_generate_derived_key_pair_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.generate_derived_key_pair, &mock,
		ECC_ENGINE_GENERATE_KEY_FAILED, MOCK_ARG (ECC_PRIVKEY), MOCK_ARG (ECC_PRIVKEY_LEN),
		MOCK_ARG (&priv_key), MOCK_ARG (&pub_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.generate_derived_key_pair (&engine.base, ECC_PRIVKEY, ECC_PRIVKEY_LEN,
		&priv_key, &pub_key);
	CuAssertIntEquals (test, ECC_ENGINE_GENERATE_KEY_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_generate_derived_key_pair_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.generate_derived_key_pair (NULL, ECC_PRIVKEY, ECC_PRIVKEY_LEN,
		&priv_key, &pub_key);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_generate_key_pair (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.generate_key_pair, &mock, 0, MOCK_ARG (&priv_key),
		MOCK_ARG (&pub_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.generate_key_pair (&engine.base, &priv_key, &pub_key);
	CuAssertIntEquals (test, 0, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_generate_key_pair_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.generate_key_pair, &mock,
		ECC_ENGINE_GENERATE_KEY_FAILED, MOCK_ARG (&priv_key), MOCK_ARG (&pub_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.generate_key_pair (&engine.base, &priv_key, &pub_key);
	CuAssertIntEquals (test, ECC_ENGINE_GENERATE_KEY_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_generate_key_pair_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.generate_key_pair (NULL, &priv_key, &pub_key);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_release_key_pair (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.release_key_pair, &mock, 0, MOCK_ARG (&priv_key),
		MOCK_ARG (&pub_key));
	CuAssertIntEquals (test, 0, status);

	engine.base.release_key_pair (&engine.base, &priv_key, &pub_key);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_release_key_pair_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	engine.base.release_key_pair (NULL, &priv_key, &pub_key);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_signature_max_length (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_public_key pub_key;
	struct ecc_private_key priv_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.get_signature_max_length, &mock, 72,
		MOCK_ARG (&priv_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_signature_max_length (&engine.base, &priv_key);
	CuAssertIntEquals (test, 72, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_signature_max_length_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.get_signature_max_length, &mock,
		ECC_ENGINE_SIG_LENGTH_FAILED, MOCK_ARG (&priv_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_signature_max_length (&engine.base, &priv_key);
	CuAssertIntEquals (test, ECC_ENGINE_SIG_LENGTH_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_signature_max_length_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_signature_max_length (NULL, &priv_key);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_private_key_der (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;
	uint8_t *der = NULL;
	size_t length;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.get_private_key_der, &mock, 0, MOCK_ARG (&priv_key),
		MOCK_ARG (&der), MOCK_ARG (&length));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_private_key_der (&engine.base, &priv_key, &der, &length);
	CuAssertIntEquals (test, 0, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_private_key_der_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;
	uint8_t *der = NULL;
	size_t length;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.get_private_key_der, &mock,
		ECC_ENGINE_PRIVATE_KEY_DER_FAILED, MOCK_ARG (&priv_key), MOCK_ARG (&der),
		MOCK_ARG (&length));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_private_key_der (&engine.base, &priv_key, &der, &length);
	CuAssertIntEquals (test, ECC_ENGINE_PRIVATE_KEY_DER_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_private_key_der_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;
	uint8_t *der = NULL;
	size_t length;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_private_key_der (NULL, &priv_key, &der, &length);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_public_key_der (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_public_key pub_key;
	int status;
	uint8_t *der = NULL;
	size_t length;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.get_public_key_der, &mock, 0, MOCK_ARG (&pub_key),
		MOCK_ARG (&der), MOCK_ARG (&length));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_public_key_der (&engine.base, &pub_key, &der, &length);
	CuAssertIntEquals (test, 0, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_public_key_der_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_public_key pub_key;
	int status;
	uint8_t *der = NULL;
	size_t length;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.get_public_key_der, &mock,
		ECC_ENGINE_PUBLIC_KEY_DER_FAILED, MOCK_ARG (&pub_key), MOCK_ARG (&der), MOCK_ARG (&length));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_public_key_der (&engine.base, &pub_key, &der, &length);
	CuAssertIntEquals (test, ECC_ENGINE_PUBLIC_KEY_DER_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_public_key_der_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_public_key pub_key;
	int status;
	uint8_t *der = NULL;
	size_t length;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_public_key_der (NULL, &pub_key, &der, &length);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_sign (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;
	uint8_t out[72];

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.sign, &mock, 72, MOCK_ARG (&priv_key),
		MOCK_ARG (SIG_HASH_TEST), MOCK_ARG (SIG_HASH_LEN), MOCK_ARG (out), MOCK_ARG (sizeof (out)));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.sign (&engine.base, &priv_key, SIG_HASH_TEST, SIG_HASH_LEN, out,
		sizeof (out));
	CuAssertIntEquals (test, 72, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_sign_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;
	uint8_t out[72];

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.sign, &mock, ECC_ENGINE_SIGN_FAILED,
		MOCK_ARG (&priv_key), MOCK_ARG (SIG_HASH_TEST), MOCK_ARG (SIG_HASH_LEN), MOCK_ARG (out),
		MOCK_ARG (sizeof (out)));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.sign (&engine.base, &priv_key, SIG_HASH_TEST, SIG_HASH_LEN, out,
		sizeof (out));
	CuAssertIntEquals (test, ECC_ENGINE_SIGN_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_sign_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;
	uint8_t out[72];

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.sign (NULL, &priv_key, SIG_HASH_TEST, SIG_HASH_LEN, out,
		sizeof (out));
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_verify (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.verify, &mock, 0, MOCK_ARG (&pub_key),
		MOCK_ARG (SIG_HASH_TEST), MOCK_ARG (SIG_HASH_LEN), MOCK_ARG (ECC_SIGNATURE_TEST),
		MOCK_ARG (ECC_SIG_TEST_LEN));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.verify (&engine.base, &pub_key, SIG_HASH_TEST, SIG_HASH_LEN,
		ECC_SIGNATURE_TEST, ECC_SIG_TEST_LEN);
	CuAssertIntEquals (test, 0, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_verify_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.verify, &mock, ECC_ENGINE_VERIFY_FAILED,
		MOCK_ARG (&pub_key), MOCK_ARG (SIG_HASH_TEST), MOCK_ARG (SIG_HASH_LEN),
		MOCK_ARG (ECC_SIGNATURE_TEST), MOCK_ARG (ECC_SIG_TEST_LEN));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.verify (&engine.base, &pub_key, SIG_HASH_TEST, SIG_HASH_LEN,
		ECC_SIGNATURE_TEST, ECC_SIG_TEST_LEN);
	CuAssertIntEquals (test, ECC_ENGINE_VERIFY_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_verify_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.verify (NULL, &pub_key, SIG_HASH_TEST, SIG_HASH_LEN,
		ECC_SIGNATURE_TEST, ECC_SIG_TEST_LEN);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_shared_secret_max_length (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.get_shared_secret_max_length, &mock,
		ECC_DH_SECRET_MAX_LENGTH, MOCK_ARG (&priv_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_shared_secret_max_length (&engine.base, &priv_key);
	CuAssertIntEquals (test, ECC_DH_SECRET_MAX_LENGTH, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_shared_secret_max_length_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.get_shared_secret_max_length, &mock,
		ECC_ENGINE_SECRET_LENGTH_FAILED, MOCK_ARG (&priv_key));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_shared_secret_max_length (&engine.base, &priv_key);
	CuAssertIntEquals (test, ECC_ENGINE_SECRET_LENGTH_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_get_shared_secret_max_length_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.get_shared_secret_max_length (NULL, &priv_key);
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_compute_shared_secret (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;
	uint8_t out[ECC_DH_SECRET_LEN];

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.compute_shared_secret, &mock, ECC_DH_SECRET_LEN,
		MOCK_ARG (&priv_key), MOCK_ARG (&pub_key), MOCK_ARG (out), MOCK_ARG (sizeof (out)));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.compute_shared_secret (&engine.base, &priv_key, &pub_key, out,
		sizeof (out));
	CuAssertIntEquals (test, ECC_DH_SECRET_LEN, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_compute_shared_secret_error (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;
	uint8_t out[ECC_DH_SECRET_LEN];

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = mock_expect (&mock.mock, mock.base.compute_shared_secret, &mock,
		ECC_ENGINE_SHARED_SECRET_FAILED, MOCK_ARG (&priv_key), MOCK_ARG (&pub_key), MOCK_ARG (out),
		MOCK_ARG (sizeof (out)));
	CuAssertIntEquals (test, 0, status);

	status = engine.base.compute_shared_secret (&engine.base, &priv_key, &pub_key, out,
		sizeof (out));
	CuAssertIntEquals (test, ECC_ENGINE_SHARED_SECRET_FAILED, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}

static void ecc_thread_safe_test_compute_shared_secret_null (CuTest *test)
{
	struct ecc_engine_thread_safe engine;
	struct ecc_engine_mock mock;
	struct ecc_private_key priv_key;
	struct ecc_public_key pub_key;
	int status;
	uint8_t out[ECC_DH_SECRET_LEN];

	TEST_START;

	status = ecc_mock_init (&mock);
	CuAssertIntEquals (test, 0, status);

	status = ecc_thread_safe_init (&engine, &mock.base);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.compute_shared_secret (NULL, &priv_key, &pub_key, out,
		sizeof (out));
	CuAssertIntEquals (test, ECC_ENGINE_INVALID_ARGUMENT, status);

	status = mock_validate (&mock.mock);
	CuAssertIntEquals (test, 0, status);

	/* Check lock has been released. */
	engine.base.init_key_pair (&engine.base, (const uint8_t*) ECC_PRIVKEY_DER, ECC_PRIVKEY_DER_LEN,
		NULL, &pub_key);

	ecc_mock_release (&mock);
	ecc_thread_safe_release (&engine);
}


CuSuite* get_ecc_thread_safe_suite ()
{
	CuSuite *suite = CuSuiteNew ();

	SUITE_ADD_TEST (suite, ecc_thread_safe_test_init);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_init_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_release_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_init_key_pair);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_init_key_pair_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_init_key_pair_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_init_public_key);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_init_public_key_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_init_public_key_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_generate_derived_key_pair);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_generate_derived_key_pair_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_generate_derived_key_pair_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_generate_key_pair);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_generate_key_pair_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_generate_key_pair_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_release_key_pair);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_release_key_pair_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_signature_max_length);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_signature_max_length_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_signature_max_length_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_private_key_der);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_private_key_der_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_private_key_der_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_public_key_der);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_public_key_der_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_public_key_der_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_sign);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_sign_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_sign_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_verify);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_verify_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_verify_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_shared_secret_max_length);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_shared_secret_max_length_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_get_shared_secret_max_length_null);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_compute_shared_secret);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_compute_shared_secret_error);
	SUITE_ADD_TEST (suite, ecc_thread_safe_test_compute_shared_secret_null);

	return suite;
}
