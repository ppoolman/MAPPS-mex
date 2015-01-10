/*=================================================================
 * _SetUpConnection_client_code.h
 *
 *=================================================================*/


//////////////////////////////////////////////////////////////////////////
//  Connect to the client state.
//    First search for an existing connection. If none is found, that
//    means we're either the first pass or MAPPS has reset the server
//    since we were last connected. If the search fails, register
//    ourselves now.
//////////////////////////////////////////////////////////////////////////

ClientStruct_ptr = prhs[0]; 

ClientData_ptr = mxGetField(ClientStruct_ptr, 0, "name");
if (ClientData_ptr == NULL)
    ClientData_ptr = mxGetField(ClientStruct_ptr, 0, "Name");
if (ClientData_ptr != NULL) { 
    if (mxIsChar(ClientData_ptr) && mxGetNumberOfDimensions(ClientData_ptr) == 2 && (mxGetM(ClientData_ptr) == 1 || mxGetN(ClientData_ptr) == 1)) {
        clientString_length = mxGetNumberOfElements(ClientData_ptr) + 1;
        if (mxGetString((const mxArray *)ClientData_ptr, ClientName, (mwSize)clientString_length) != 0) {
            mexPrintf("Could not convert struct data into client name.  Using default name...\n");
            ClientName_ptr = CLIENTNAME;
        }
        else
            ClientName_ptr = ClientName;
    }
    else {
        mexPrintf("Incorrect type and/or formatting of struct data for client name.  Using default name...\n");
        ClientName_ptr = CLIENTNAME;
    }
}
else
    ClientName_ptr = CLIENTNAME;

client = FindClientByName(ClientName_ptr, global);

if(client == NULL)
{
/*    ClientData_ptr = mxGetField(ClientStruct_ptr, 0, "type");
    if (ClientData_ptr != NULL) { 
        if (mxIsChar(ClientData_ptr) && mxGetNumberOfDimensions(ClientData_ptr) == 2 && (mxGetM(ClientData_ptr) == 1 || mxGetN(ClientData_ptr) == 1)) {
            clientString_length = mxGetNumberOfElements(ClientData_ptr) + 1;
            if (mxGetString((const mxArray *)ClientData_ptr, ClientType, (mwSize)clientString_length) != 0) {
                mexPrintf("Could not convert struct data into client type.  Using default type...\n");
                ClientType_ptr = CLIENTTYPE;
            }
            else
                ClientType_ptr = ClientType;
        }
        else {
            mexPrintf("Incorrect type and/or formatting of struct data for client type.  Using default type...\n");
            ClientType_ptr = CLIENTTYPE;
        }
    }
    else
        ClientType_ptr = CLIENTTYPE;*/
    ClientType_ptr = CLIENTTYPE;

    ClientData_ptr = mxGetField(ClientStruct_ptr, 0, "attributes");
    if (ClientData_ptr != NULL) { 
        if (mxIsChar(ClientData_ptr) && mxGetNumberOfDimensions(ClientData_ptr) == 2 && (mxGetM(ClientData_ptr) == 1 || mxGetN(ClientData_ptr) == 1)) {
            clientString_length = mxGetNumberOfElements(ClientData_ptr) + 1;
            if (mxGetString((const mxArray *)ClientData_ptr, ClientAttributes, (mwSize)clientString_length) != 0) {
                mexPrintf("Could not convert struct data into client attributes.  Using default attributes...\n");
                ClientAttributes_ptr = CLIENTATTRIBUTES;
            }
            else
                ClientAttributes_ptr = ClientAttributes;
        }
        else {
            mexPrintf("Incorrect type and/or formatting of struct data for client attributes.  Using default attributes...\n");
            ClientAttributes_ptr = CLIENTATTRIBUTES;
        }
    }
    else
        ClientAttributes_ptr = CLIENTATTRIBUTES;

    ClientData_ptr = mxGetField(ClientStruct_ptr, 0, "shmemName");
    if (ClientData_ptr != NULL) { 
        if (mxIsChar(ClientData_ptr) && mxGetNumberOfDimensions(ClientData_ptr) == 2 && (mxGetM(ClientData_ptr) == 1 || mxGetN(ClientData_ptr) == 1)) {
            clientString_length = mxGetNumberOfElements(ClientData_ptr) + 1;
            if (mxGetString((const mxArray *)ClientData_ptr, ClientShmemName, (mwSize)clientString_length) != 0) {
                mexPrintf("Could not convert struct data into client shared memory name.  Using default shared memory name...\n");
                ClientShmemName_ptr = CLIENTSHMEMNAME;
            }
            else
                ClientShmemName_ptr = ClientShmemName;
        }
        else {
            mexPrintf("Incorrect type and/or formatting of struct data for client shared memory name.  Using shared memory name...\n");
            ClientShmemName_ptr = CLIENTSHMEMNAME;
        }
    }
    else
        ClientShmemName_ptr = CLIENTSHMEMNAME;

    ClientData_ptr = mxGetField(ClientStruct_ptr, 0, "shmemSize");
    if (ClientData_ptr != NULL) { 
        if (mxIsNumeric(ClientData_ptr) && mxGetNumberOfDimensions(ClientData_ptr) == 2 && mxGetM(ClientData_ptr) == 1 && mxGetN(ClientData_ptr) == 1) {
            clientShmemSize = (int)mxGetScalar(ClientData_ptr);
            if (clientShmemSize <= 0) {
                mexPrintf("Could not convert struct data into client shared memory size.  Using default shared memory size...\n");
                clientShmemSize = CLIENTSHMEMSIZE;
            }
        }
        else {
            mexPrintf("Incorrect type and/or formatting of struct data for client shared memory size.  Using shared memory size...\n");
            clientShmemSize = CLIENTSHMEMSIZE;
        }
    }
    else             
        clientShmemSize = CLIENTSHMEMSIZE;

    //mexPrintf("Name       = %s\n", ClientName_ptr);
    //mexPrintf("Type       = %s\n", ClientType_ptr);
    //mexPrintf("Attributes = %s\n", ClientAttributes_ptr);
    //mexPrintf("ShmemName  = %s\n", ClientShmemName_ptr);
    //mexPrintf("clientShmemSize = %d\n", clientShmemSize);

    memset(&clientDetails, 0, sizeof(clientDetails));
    
    sprintf(clientDetails.Name, ClientName_ptr);
    sprintf(clientDetails.Type, ClientType_ptr);
    sprintf(clientDetails.Attributes, ClientAttributes_ptr);
    sprintf(clientDetails.ShmemName, ClientShmemName_ptr);
    clientDetails.ShmemSize = clientShmemSize;

    client = RegisterClient(&clientDetails, global);
}

if(client == NULL)
{
    DisconnectFromGlobal(h1, global);
    mexErrMsgTxt("There are no empty client slots available! \n");
}
else
    client->HeartBeat++;