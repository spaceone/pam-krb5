/*
 * Tests for the alt_auth_map functionality in libpam-krb5.
 *
 * This test case tests the variations of the alt_auth_map functionality for
 * both authentication and account management.  It requires a Kerberos
 * configuration, but does not attempt to save a session ticket cache (to
 * avoid requiring user configuration).
 *
 * Written by Russ Allbery <rra@stanford.edu>
 * Copyright 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/system.h>

#include <tests/fakepam/script.h>
#include <tests/tap/kerberos.h>
#include <tests/tap/process.h>
#include <tests/tap/string.h>


int
main(void)
{
    struct script_config config;
    struct kerberos_config *krbconf;

    /*
     * Load the Kerberos principal and password from a file, but set the
     * principal as extra[0] and use something else bogus as the user.  We
     * want to test that alt_auth_map works when there's no relationship
     * between the mapped principal and the user.
     */
    krbconf = kerberos_setup(TAP_KRB_NEEDS_PASSWORD);
    memset(&config, 0, sizeof(config));
    config.user = "bogus-nonexistent-account";
    config.authtok = krbconf->password;
    config.extra[0] = krbconf->username;
    config.extra[1] = krbconf->userprinc;

    /*
     * Generate a testing krb5.conf file with a nonexistent default realm so
     * that we can be sure that our principals will stay fully-qualified in
     * the logs.
     */
    kerberos_generate_conf("bogus.example.com");
    config.extra[2] = "bogus.example.com";

    /* Test without password prompting. */
    plan_lazy();
    run_script("data/scripts/alt-auth/basic", &config);
    run_script("data/scripts/alt-auth/basic-debug", &config);
    run_script("data/scripts/alt-auth/fail", &config);
    run_script("data/scripts/alt-auth/fail-debug", &config);
    run_script("data/scripts/alt-auth/force", &config);
    run_script("data/scripts/alt-auth/only", &config);

    /*
     * If the alternate account exists but the password is incorrect, we
     * should not fall back to the regular account.  Test with debug so that
     * we don't need two principals configured.
     */
    config.authtok = "bogus incorrect password";
    run_script("data/scripts/alt-auth/force-fail-debug", &config);

    /*
     * Switch to our correct user (but wrong realm) realm to test username
     * mapping.  The second test splits the username into two parts, one in
     * the PAM configuration and one in the real username, so that we can test
     * interpolation of the username when %s isn't the first token.
     */
    config.authtok = krbconf->password;
    config.user = krbconf->username;
    config.extra[2] = krbconf->realm;
    run_script("data/scripts/alt-auth/username-map", &config);
    config.user = &krbconf->username[1];
    config.extra[3] = bstrndup(krbconf->username, 1);
    run_script("data/scripts/alt-auth/username-map-prefix", &config);
    free((char *) config.extra[3]);
    config.extra[3] = NULL;

    /*
     * Add the password and make the user match our authentication principal,
     * and then test fallback to normal authentication when alternative
     * authentication fails.
     */
    config.user = krbconf->userprinc;
    config.password = krbconf->password;
    config.extra[2] = krbconf->realm;
    run_script("data/scripts/alt-auth/fallback", &config);
    run_script("data/scripts/alt-auth/fallback-debug", &config);
    run_script("data/scripts/alt-auth/force-fallback", &config);
    run_script("data/scripts/alt-auth/only-fail", &config);

    return 0;
}