#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#define FAIL    -1

void delay(int number_of_seconds);


// Create the SSL socket and intialize the socket address structure
int OpenListener(int port)
{
    int sd;
    struct sockaddr_in addr;
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        perror("can't bind port");
        abort();
    }
    if ( listen(sd, 10) != 0 )
    {
        perror("Can't configure listening port");
        abort();
    }
    return sd;
}
int isRoot()
{
    if (getuid() != 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
SSL_CTX* InitServerCTX(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
    //method = TLSv1_2_server_method();  /* create new server-method instance */
    method = TLS_server_method();
    ctx = SSL_CTX_new(method);   /* create new context from method */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates.\n");
}
void Servlet(SSL* ssl) /* Serve the connection -- threadable */
{
    char buf[1024] = {0};
    int sd, bytes;
    const char* ServerResponse="<\Body>\
                               <Name>aticleworld.com</Name>\
                 <year>1.5</year>\
                 <BlogType>Embedede and c\c++<\BlogType>\
                 <Author>amlendra<Author>\
                 <\Body>";
    const char *cpValidMessage = "<Body>\
                               <UserName>alice<UserName>\
                 <Password>alice<Password>\
                 <\Body>";
    if ( SSL_accept(ssl) == FAIL )     /* do SSL-protocol accept */
        ERR_print_errors_fp(stderr);
    else
    {
        ShowCerts(ssl);        /* get any certificates */
        bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
        buf[bytes] = '\0';
        printf("Client msg: \"%s\"\n", buf);
        if ( bytes > 0 )
        {
            if(strcmp(cpValidMessage,buf) == 0)
            {
                SSL_write(ssl, ServerResponse, strlen(ServerResponse)); /* send reply */
            }
            else
            {
                SSL_write(ssl, "Invalid Message", strlen("Invalid Message")); /* send reply */
            }
        }
        else
        {
            ERR_print_errors_fp(stderr);
        }
    }
    send_authentication_msg(ssl);
    sd = SSL_get_fd(ssl);       /* get socket connection */
    SSL_free(ssl);         /* release SSL state */
    close(sd);          /* close connection */
}

void send_authentication_msg(SSL* ssl)
{

   int ret; // used for return values

   // read my private key file from keyboard:
   //string prvkey_file_name;
   //cout << "Please, type the PEM file containing my private key: ";
   //getline(cin, prvkey_file_name);
   //if(!cin) { cerr << "Error during input\n"; exit(1); }

   // load my private key: MessageApp_key.pem
   FILE* prvkey_file = fopen("MessageApp_key.pem", "r");
   if(!prvkey_file){ printf("Error: cannot open key file\n");}
   EVP_PKEY* prvkey = PEM_read_PrivateKey(prvkey_file, NULL, NULL, NULL);
   fclose(prvkey_file);
   if(!prvkey){ printf("Error: private key returned NULL\n");}

   // read the file to sign from keyboard:
   // string clear_file_name;
   // cout << "Please, type the file to sign: ";
   // getline(cin, clear_file_name);
   // if(!cin) { cerr << "Error during input\n"; exit(1); }
   // 
   // // open the file to sign:
   // FILE* clear_file = fopen(clear_file_name.c_str(), "rb");
   // if(!clear_file) { cerr << "Error: cannot open file '" << clear_file_name << "' (file does not exist?)\n"; exit(1); }
   // 
   // // get the file size: 
   // // (assuming no failures in fseek() and ftell())
   // fseek(clear_file, 0, SEEK_END);
   // long int clear_size = ftell(clear_file);
   // fseek(clear_file, 0, SEEK_SET);
   // 
   // // read the plaintext from file:
   // unsigned char* clear_buf = (unsigned char*)malloc(clear_size);
   // if(!clear_buf) { cerr << "Error: malloc returned NULL (file too big?)\n"; exit(1); }
   // ret = fread(clear_buf, 1, clear_size, clear_file);
   // if(ret < clear_size) { cerr << "Error while reading file '" << clear_file_name << "'\n"; exit(1); }
   // fclose(clear_file);

   unsigned char* clear_buf = "authenticationOK";//(unsigned char*)malloc(16*sizeof(char));
   // declare some useful variables:
   const EVP_MD* md = EVP_sha256();

   // create the signature context:
   EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
   if(!md_ctx){ printf("Error: EVP_MD_CTX_new returned NULL\n"); }

   // allocate buffer for signature:
   unsigned char* sgnt_buf = (unsigned char*)malloc(EVP_PKEY_size(prvkey));
   if(!sgnt_buf) { printf("Error: malloc returned NULL (signature too big?)\n"); }

   // sign the plaintext:
   // (perform a single update on the whole plaintext, 
   // assuming that the plaintext is not huge)
   ret = EVP_SignInit(md_ctx, md);
   if(ret == 0){ printf("Error: EVP_SignInit returned %d\n",ret); }
   ret = EVP_SignUpdate(md_ctx, clear_buf, strlen(clear_buf));
   if(ret == 0){ printf("Error: EVP_SignUpdate returned %d\n", ret);}
   unsigned int sgnt_size;
   ret = EVP_SignFinal(md_ctx, sgnt_buf, &sgnt_size, prvkey);
   if(ret == 0){ printf("Error: EVP_SignFinal returned %d\n", ret);}

   // delete the digest and the private key from memory:
   EVP_MD_CTX_free(md_ctx);
   EVP_PKEY_free(prvkey);

   // write the signature into a '.sgn' file:
   // string sgnt_file_name = clear_file_name + ".sgn";
   // FILE* sgnt_file = fopen(sgnt_file_name.c_str(), "wb");
   // if(!sgnt_file) { cerr << "Error: cannot open file '" << sgnt_file_name << "' (no permissions?)\n"; exit(1); }
   // ret = fwrite(sgnt_buf, 1, sgnt_size, sgnt_file);
   // if(ret < sgnt_size) { cerr << "Error while writing the file '" << sgnt_file_name << "'\n"; exit(1); }
   // fclose(sgnt_file);
   // 
   // cout << "File '"<< clear_file_name << "' signed into file '" << sgnt_file_name << "'\n";
   sleep(10);
   printf("message: %s\n", clear_buf);
   printf("signed message:\n %s\n", sgnt_buf);
   SSL_write(ssl, clear_buf, strlen(clear_buf));
   SSL_write(ssl, sgnt_buf, strlen(sgnt_buf));
   // deallocate buffers:
   //free(clear_buf);
   free(sgnt_buf);

   return 0;
   
}


void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;
  
    // Storing start time
    clock_t start_time = clock();
  
    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds);
}


int main(int count, char *Argc[])
{
    SSL_CTX *ctx;
    int server;
    char *portnum;
//Only root user have the permsion to run the server
    if(!isRoot())
    {
        printf("This program must be run as root/sudo user!!");
        exit(0);
    }
    if ( count != 2 )
    {
        printf("Usage: %s <portnum>\n", Argc[0]);
        exit(0);
    }
    // Initialize the SSL library
    SSL_library_init();
    portnum = Argc[1];
    ctx = InitServerCTX();        /* initialize SSL */
    LoadCertificates(ctx, "MessageApp_cert.pem", "MessageApp_key.pem"); /* load certs */
    server = OpenListener(atoi(portnum));    /* create server socket */
    while (1)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        SSL *ssl;
        int client = accept(server, (struct sockaddr*)&addr, &len);  /* accept connection as usual */
        printf("Connection: %s:%d\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));        
        ssl = SSL_new(ctx);              /* get new SSL state with context */
        SSL_set_fd(ssl, client);      /* set connection socket to SSL state */
        
        Servlet(ssl);         /* service connection */
        
        
        
    }
    close(server);          /* close server socket */
    SSL_CTX_free(ctx);         /* release context */
} 