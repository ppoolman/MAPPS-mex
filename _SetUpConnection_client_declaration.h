/*=================================================================
 * _SetUpConnection_client_declaration.h
 *
 *=================================================================*/

#define CLIENTNAME "matlab"
#define CLIENTTYPE "matlab"
#define CLIENTATTRIBUTES "Some useful info goes here..."
#define CLIENTSHMEMNAME "Local\\mapps.data_server.clients.matlab"
#define CLIENTSHMEMSIZE 25 * 1024 * 1024

int clientShmemSize;
size_t clientString_length;
char ClientName[128], *ClientName_ptr, 
     ClientType[32], *ClientType_ptr,
     ClientAttributes[128], *ClientAttributes_ptr,
     ClientShmemName[128], *ClientShmemName_ptr;
const mxArray *ClientStruct_ptr, *ClientData_ptr;
void* h2 = NULL;
EDS_Client clientDetails;
EDS_Client* client; 