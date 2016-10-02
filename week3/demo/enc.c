#include <mcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main()
{
MCRYPT td;
  int i;
  char *key;
  char password[20];
  char block_buffer;
  char *IV;
  int keysize=19; /* 128 bits */

  key=calloc(1, keysize);
  strcpy(password, "A_large_key");

/* Generate the key using the password */
/*  mhash_keygen( KEYGEN_MCRYPT, MHASH_MD5, key, keysize, NULL, 0, password, strlen(password));
 */
  memmove( key, password, strlen(password));

  td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  if (td==MCRYPT_FAILED) {
     return 1;
  }
  IV = malloc(mcrypt_enc_get_iv_size(td));
  printf("%d\n", mcrypt_enc_get_iv_size(td));
/* Put random data in IV. Note these are not real random data, 
 * consider using /dev/random or /dev/urandom.
 */

  /*  srand(time(0)); */
  for (i=0; i< mcrypt_enc_get_iv_size( td); i++) {
    IV[i]=rand();
  }

  /* Encryption in CFB is performed in bytes */
  mcrypt_generic_deinit(td);

  mcrypt_module_close(td);
  unsigned int seed = 10095;
  srand(seed);
  int j, r;
  for (j = 0; j < 10; j++) {
	  r = rand();
	  printf("%d\n", r);
  }
  return 0;

}
