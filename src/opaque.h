#ifndef opaque_h
#define opaque_h

#include <stdint.h>
#include <stdlib.h>
#include "decaf.h"
#include <crypto_secretbox.h>

#define OPAQUE_BLOB_LEN (crypto_secretbox_NONCEBYTES+DECAF_X25519_PRIVATE_BYTES+DECAF_X25519_PUBLIC_BYTES+DECAF_X25519_PUBLIC_BYTES+crypto_secretbox_MACBYTES)
#define OPAQUE_USER_RECORD_LEN (DECAF_255_SCALAR_BYTES+DECAF_X25519_PRIVATE_BYTES+DECAF_X25519_PUBLIC_BYTES+DECAF_X25519_PUBLIC_BYTES+32+sizeof(uint64_t)+OPAQUE_BLOB_LEN)
#define OPAQUE_USER_SESSION_PUBLIC_LEN (DECAF_X25519_PUBLIC_BYTES+DECAF_X25519_PUBLIC_BYTES)
#define OPAQUE_USER_SESSION_SECRET_LEN (DECAF_X25519_PRIVATE_BYTES+DECAF_X25519_PRIVATE_BYTES)
#define OPAQUE_SERVER_SESSION_LEN (DECAF_X25519_PUBLIC_BYTES+DECAF_X25519_PUBLIC_BYTES+DECAF_X25519_PUBLIC_BYTES+32+sizeof(uint64_t)+OPAQUE_BLOB_LEN)
#define OPAQUE_REGISTER_PUBLIC_LEN (DECAF_X25519_PUBLIC_BYTES+DECAF_X25519_PUBLIC_BYTES)
#define OPAQUE_REGISTER_SECRET_LEN (DECAF_X25519_PRIVATE_BYTES+DECAF_X25519_PRIVATE_BYTES)

/*
   This function implements the storePwdFile function from the
   paper. This function runs on the server and creates a new output
   record rec of secret key material and optional extra data partly
   encrypted with a key derived from the input password pw. The server
   needs to implement the storage of this record and any binding to
   user names or as the paper suggests sid.  *Attention* the size of
   rec depends on the size of extra data provided.
 */
int opaque_init_srv(const uint8_t *pw, const size_t pwlen, const unsigned char *extra, const uint64_t extra_len, unsigned char rec[OPAQUE_USER_RECORD_LEN]);

/*
  This function initiates a new OPAQUE session, is the same as the function
  defined in the paper with the usrSession name. The User initiates a new session by
  providing its input password pw, and receving a private sec and a "public"
  pub output parameter. The User should protect the sec value until later in
  the protocol and send the pub value over to the Server.
 */
void opaque_session_usr_start(const uint8_t *pw, const size_t pwlen, unsigned char sec[OPAQUE_USER_SESSION_SECRET_LEN], unsigned char pub[OPAQUE_USER_SESSION_PUBLIC_LEN]);

/*
  This is the same function as defined in the paper with the srvSession name. It runs
  on the server and receives the output pub from the user running usrSession(),
  futhermore the server needs to load the user record created when registering
  the user with the storePwdFile() function. These input parameters are
  transformed into a secret/shared session key sk and a response resp to be
  sent back to the user. *Attention* rec and resp have variable length
  depending on any extra data stored.
 */
int opaque_session_srv(const unsigned char pub[OPAQUE_USER_SESSION_PUBLIC_LEN], const unsigned char rec[OPAQUE_USER_RECORD_LEN], unsigned char resp[OPAQUE_SERVER_SESSION_LEN], uint8_t *sk);

/*
 This is the same function as defined in the paper with the usrSessionEnd name. It is
 run by the user, and recieves as input the response from the previous server
 srvSession() function as well as the sec value from running the usrSession()
 function that initiated this protocol, the user password pw is also needed as
 an input to this final step. All these input parameters are transformed into a
 shared/secret session key pk, which should be the same as the one calculated
 by the srvSession() function. *Attention* resp has a length depending on extra
 data. If rwd is not NULL it is returned - this enables to run the sphinx protocol
 in the opaque protocol.
*/
int opaque_session_usr_finish(const uint8_t *pw, const size_t pwlen, const unsigned char resp[OPAQUE_SERVER_SESSION_LEN], const unsigned char sec[OPAQUE_USER_SESSION_SECRET_LEN], uint8_t *sk, uint8_t *extra, uint8_t rwd[crypto_secretbox_KEYBYTES]);

/*
 * This is a simple utility function that can be used to calculate
 * f_k(c), where c is a constant, this is useful if the peers want to
 * authenticate each other.
 */
void opaque_f(const uint8_t *k, const size_t k_len, const uint8_t val, uint8_t *res);
//todo: opaque_session_srv3

/* Alternative user initialization
 *
 * The paper originally proposes a very simple 1 shot interface for
 * registering a new "user", however this has the drawback that in
 * that case the users secrets and its password are exposed in
 * cleartext at registration to the server. There is a much less
 * efficient 4 message registration protocol which avoids the exposure
 * of the secrets and the password to the server which can be
 * instantiated by the following for registration functions:
 */


/*
 * The user inputs its password pw, and receives an ephemeral secret r
 * and a blinded value alpha as output. r should be protected until
 * step 3 of this registration protocol and the value alpha should be
 * passed to the server.
 */
void opaque_private_init_usr_start(const uint8_t *pw, const size_t pwlen, uint8_t *r, uint8_t *alpha);

/*
 * The server receives alpha from the users invocation of its
 * newUser() function, it outputs a value sec which needs to be
 * protected until step 4 by the server. This function also outputs a
 * value pub which needs to be passed to the user.
 */
int opaque_private_init_srv_respond(const uint8_t *alpha, unsigned char sec[OPAQUE_REGISTER_SECRET_LEN], unsigned char pub[OPAQUE_REGISTER_PUBLIC_LEN]);

/*
 * This function is run by the user, taking as input the users password pw, the
 * ephemeral secret r that was an output of the user running newUser(), and the
 * output pub from the servers run of initUser(). Futhermore the
 * extra/extra_len parameter can be used to store additional data in the
 * encrypted user record. The result of this is the value rec which should be
 * passed for the last step to the server. *Attention* the size of rec depends
 * on extra data length. If rwd is not NULL it is returned - this enables to run
 * the sphinx protocol in the opaque protocol.
 */
int opaque_private_init_usr_respond(const uint8_t *pw, const size_t pwlen, const uint8_t *r, const unsigned char pub[OPAQUE_REGISTER_PUBLIC_LEN], const unsigned char *extra, const uint64_t extra_len, unsigned char rec[OPAQUE_USER_RECORD_LEN], uint8_t rwd[crypto_secretbox_KEYBYTES]);

/*
 * The server combines the sec value from its run of its initUser() function
 * with the rec output of the users registerUser() function, creating the final
 * record, which should be the same as the output of the 1-step storePwdFile()
 * init function of the paper. The server should save this record in
 * combination with a user id and/or sid value as suggested in the paper.
 * *Attention* the size of rec depends on extra data length.
 */
//
void opaque_private_init_srv_finish(const unsigned char sec[OPAQUE_REGISTER_SECRET_LEN], const unsigned char pub[OPAQUE_REGISTER_PUBLIC_LEN], unsigned char rec[OPAQUE_USER_RECORD_LEN]);

#endif // opaque_h
