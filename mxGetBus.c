/*=================================================================
 * mxGetBus.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

mxClassID Interpret_MAPPS_DataType(const char *DataType)
{
    if (strcmp(DataType, "uchar") == 0)			return mxUINT8_CLASS;
    else if (strcmp(DataType, "int16") == 0)	return mxINT16_CLASS;
    else if (strcmp(DataType, "uint16") == 0) 	return mxUINT16_CLASS;
    else if (strcmp(DataType, "int") == 0) 		return mxINT32_CLASS;
    else if (strcmp(DataType, "uint") == 0) 	return mxUINT32_CLASS;
    else if (strcmp(DataType, "int64") == 0) 	return mxINT64_CLASS;
    else if (strcmp(DataType, "uint64") == 0) 	return mxUINT64_CLASS;
    else if (strcmp(DataType, "float") == 0) 	return mxSINGLE_CLASS;
    else if (strcmp(DataType, "double") == 0) 	return mxDOUBLE_CLASS;
    else if (strcmp(DataType, "string") == 0) 	return mxCHAR_CLASS;  
    else 										return mxUNKNOWN_CLASS;
}

void Retrieve_MAPPS_Data_Multiple_Fields(
	const char *Busname, 
    const int nFieldnames_requested, 
	const char **Fieldnames_requested,   
	const int64 fromTime, 
	const int64 toTime,
        
	int *nFields,
    char ***Fieldnames,
    void ***Data, 
	char ***DataType,
    int **BytesPerEntry,
    int ***EntriesPerTimeslice,   // for bus with variable-length blobs per timeslice
        
	void **Timestamps, 
	char **Timestamp_DataType, 
	uint64 *nTimeslices,
        
	void *global, void *client, void *h1, void *h2, void *Buffer)
{
    /* 1) Search for field "Fieldname" in the MAPPS buffer "Busname"
     * 2) If available, copy elements between "fromTime" and "toTime" into vector, and pass back in "Data"
     * 3) Report data type (single, double, uint64, char, etc.) in "DataType"
     * 4) If Timestamps != NULL (i.e., associated timestamps requested), 
     *    copy MAPPS timestamps between "fromTime" and "toTime" into vector, and pass back in "Timestamps"
     * 5) If Timestamps != NULL (i.e., associated timestamps requested), 
     *    report timestamp data type (single, double, uint64, char, etc.) in "Timestamp_DataType"
     * 6) Report number of timeslices in "nTimeslices" */
    
    int i, j;
    size_t bufferSize, name_length;
    char **elementString;
    DS_RetrieveDataRequest *req; 
	DS_RetrieveDataResponse *resp;
    char* payload;

	*nTimeslices = 0;
    
    //printf("\nBusname = %s\n", Busname);
    //printf("Fieldname = %s\n", Fieldname);
    //printf("fromTime = %lld\n", fromTime);
    //printf("toTime = %lld\n", toTime);


    //////////////////////////////////////////////////////////////////////////
	// (1) Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare((EDS_GlobalState *)global, (EDS_Client *)client, EDS_RetrieveBuses) == FALSE)
	{
		DisconnectFromClient(h2, (EDS_Client *)client);
        DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
		mexErrMsgIdAndTxt("MEX:GetBus:ClientBlocked", "Server is still blocking the client channel! \n");
	}
    

	//////////////////////////////////////////////////////////////////////////
	// (2) Populate
	//////////////////////////////////////////////////////////////////////////

	req = (DS_RetrieveDataRequest *)Buffer;
    bufferSize = 0;
    
	sprintf(req->BusName, Busname);
	if (nFieldnames_requested == 0)
        sprintf(req->ElementNames, "");
    else if (nFieldnames_requested > 0 && Fieldnames_requested != NULL)
    {
        sprintf(req->ElementNames, "%s", Fieldnames_requested[0]);
        for (i=1; i<nFieldnames_requested; i++)
        {
            if (strlen(req->ElementNames) + strlen(Fieldnames_requested[i]) + 1 + 1 < sizeof(req->ElementNames))
            {
                strcat(req->ElementNames, ",");
                strcat(req->ElementNames, Fieldnames_requested[i]);
            }
        }
    }
    else
        sprintf(req->ElementNames, "*");
	req->StartTimeMilli = fromTime;
	req->EndTimeMilli = toTime;
	req->IncludeIndicies = (Timestamps!=NULL);
    
    bufferSize += sizeof(DS_RetrieveDataRequest);
           
    
    //////////////////////////////////////////////////////////////////////////
	// (3) Execute
	//////////////////////////////////////////////////////////////////////////
    
    if(Execute((EDS_GlobalState *)global, (EDS_Client *)client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, (EDS_Client *)client);
        DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
        mexErrMsgIdAndTxt("MEX:GetBus:ServerTimedOut", "SERVER TIMED OUT! \n");
    }


	//////////////////////////////////////////////////////////////////////////
	// (3) Populate the data type and actual data
	//////////////////////////////////////////////////////////////////////////

    // IsVariableEncoded: 1 if true, 0 if false
    // SizeInBytes: If variable encoded, this is the size of the ENTIRE blob (all data across all reported timeslices). Otherwise, it's the size of a single element (i.e., entry or value).
    // PointerToTimeStamps: Offset from the base payload to get to the beginning of the timestamps. Set to '-1' if no timestamps are included.
    // PointerToFrameSizes: An array of pointers to get from the base payload to the respective frame size arrays. Always '-1' for non-variable encoded fields. If variable, it will populate up to ElementCount (2 for stereo audio)
    // StartOfData: An array of pointers to the beginning of the actual data. One field per element.
    
    
	//mexPrintf("%s\n", ((EDS_Client *)client)->Result); 
    if(strncmp(((EDS_Client *)client)->Result, "success", 7) == 0)
	{
		resp = (DS_RetrieveDataResponse *)Buffer;
        
        *nTimeslices = resp->FrameCount;
        
        *nFields = resp->ElementCount;
      
        if (Data != NULL && *nTimeslices > 0 && *nFields > 0)
        {
            *Fieldnames = (char **)mxCalloc((mwSize)*nFields, (mwSize)sizeof(char *));
            *Data = (void **)mxCalloc((mwSize)*nFields, (mwSize)sizeof(void *));
            *DataType = (char **)mxCalloc((mwSize)*nFields, (mwSize)sizeof(char *));
            *BytesPerEntry = (int *)mxCalloc((mwSize)*nFields, (mwSize)sizeof(int));
            *EntriesPerTimeslice = (int **)mxCalloc((mwSize)*nFields, (mwSize)sizeof(int *));
           
            for (i=0; i<*nFields; i++)
            {   
                payload = &resp->StartOfPayload + resp->StartOfData[i];   // Gets a pointer to the start of the element data payload

                name_length = strlen(resp->ElementNames[i]) + 1;                    
                (*Fieldnames)[i] = (char *)mxCalloc((mwSize)name_length, (mwSize)sizeof(char));
                memcpy((*Fieldnames)[i], resp->ElementNames[i], name_length);

                (*DataType)[i] = (char *)mxCalloc(16, (mwSize)sizeof(char));
                
                if (resp->LoadedSuccessfully[i] > 0)
                {                                    
                    switch(resp->Types[i])
                    {
                        case MappsTypeInt16:
                            if ((*DataType)[i] != NULL)
                                strcpy((*DataType)[i], "int16");
                            if (*BytesPerEntry != NULL)
                                (*BytesPerEntry)[i] = (int)sizeof(int16);
                            if (resp->IsVariableEncoded == 1)
                            {
                                (*Data)[i] = (void*)mxCalloc(1, (mwSize)resp->SizeInBytes[i]);
                                memcpy((*Data)[i], payload, (size_t)resp->SizeInBytes[i]);
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int));
                                memcpy((*EntriesPerTimeslice)[i], &resp->StartOfPayload + resp->PointerToFrameSizes[i], (size_t)((*nTimeslices)*sizeof(int)));
                                for(j=0; j<*nTimeslices; j++)
                                {
                                    (*EntriesPerTimeslice)[i][j] /= (int)sizeof(int16);
                                }
                            }
                            else 
                            {
                                (*Data)[i] = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)resp->SizeInBytes[i]);
                                memcpy((*Data)[i], payload, (size_t)((*nTimeslices)*sizeof(int16)));
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc(1, (mwSize)sizeof(int));
                                (*EntriesPerTimeslice)[i][0] = -1;
                            }
                            break;

                        case MappsTypeInt:
                            if ((*DataType)[i] != NULL)
                                strcpy((*DataType)[i], "int");
                            if (*BytesPerEntry != NULL)
                                (*BytesPerEntry)[i] = (int)sizeof(int);
                            if (resp->IsVariableEncoded == 1)
                            {
                                (*Data)[i] = (void*)mxCalloc(1, (mwSize)resp->SizeInBytes[i]);
                                memcpy((*Data)[i], payload, (size_t)resp->SizeInBytes[i]);
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int));
                                memcpy((*EntriesPerTimeslice)[i], &resp->StartOfPayload + resp->PointerToFrameSizes[i], (size_t)((*nTimeslices)*sizeof(int)));
                                for(j=0; j<*nTimeslices; j++)
                                {
                                    (*EntriesPerTimeslice)[i][j] /= (int)sizeof(int);
                                }
                            }
                            else 
                            {
                                (*Data)[i] = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)resp->SizeInBytes[i]);
                                memcpy((*Data)[i], payload, (size_t)((*nTimeslices)*sizeof(int)));
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc(1, (mwSize)sizeof(int));
                                (*EntriesPerTimeslice)[i][0] = -1;
                            }
                            break;

                        case MappsTypeInt64:
                            if ((*DataType)[i] != NULL)
                                strcpy((*DataType)[i], "int64");
                            if (*BytesPerEntry != NULL)
                                (*BytesPerEntry)[i] = (int)sizeof(int64);
                            if (resp->IsVariableEncoded == 1)
                            {
                                (*Data)[i] = (void*)mxCalloc(1, (mwSize)resp->SizeInBytes[i]);
                                memcpy((*Data)[i], payload, (size_t)resp->SizeInBytes[i]);
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int));
                                memcpy((*EntriesPerTimeslice)[i], &resp->StartOfPayload + resp->PointerToFrameSizes[i], (size_t)((*nTimeslices)*sizeof(int)));
                                for(j=0; j<*nTimeslices; j++)
                                {
                                    (*EntriesPerTimeslice)[i][j] /= (int)sizeof(int64);
                                }
                            }
                            else
                            {
                                (*Data)[i] = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)resp->SizeInBytes[i]);
                                memcpy((*Data)[i], payload, (size_t)((*nTimeslices)*sizeof(int64)));
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc(1, (mwSize)sizeof(int));
                                (*EntriesPerTimeslice)[i][0] = -1;
                            }
                            break;

                        case MappsTypeFloat:
                            if ((*DataType)[i] != NULL)
                                strcpy((*DataType)[i], "float");
                            if (*BytesPerEntry != NULL)
                                (*BytesPerEntry)[i] = (int)sizeof(float);
                            if (resp->IsVariableEncoded == 1)
                            {
                                (*Data)[i] = (void*)mxCalloc(1, (mwSize)resp->SizeInBytes[i]);
                                memcpy((*Data)[i], payload, (size_t)resp->SizeInBytes[i]);
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int));
                                memcpy((*EntriesPerTimeslice)[i], &resp->StartOfPayload + resp->PointerToFrameSizes[i], (size_t)((*nTimeslices)*sizeof(int)));
                                for(j=0; j<*nTimeslices; j++)
                                {
                                    (*EntriesPerTimeslice)[i][j] /= (int)sizeof(float);
                                }
                            }
                            else
                            {
                                (*Data)[i] = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)resp->SizeInBytes[i]);                                
                                memcpy((*Data)[i], payload, (size_t)((*nTimeslices)*sizeof(float)));                                
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc(1, (mwSize)sizeof(int));                                
                                (*EntriesPerTimeslice)[i][0] = -1;
                            }
                            break;

                        case MappsTypeDouble:
                            if ((*DataType)[i] != NULL)
                                strcpy((*DataType)[i], "double");
                            if (*BytesPerEntry != NULL)
                                (*BytesPerEntry)[i] = (int)sizeof(double);
                            if (resp->IsVariableEncoded == 1)
                            {
                                (*Data)[i] = (void*)mxCalloc(1, (mwSize)resp->SizeInBytes[i]);
                                memcpy((*Data)[i], payload, (size_t)resp->SizeInBytes[i]);
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int));
                                memcpy((*EntriesPerTimeslice)[i], &resp->StartOfPayload + resp->PointerToFrameSizes[i], (size_t)((*nTimeslices)*sizeof(int)));
                                for(j=0; j<*nTimeslices; j++)
                                {
                                    (*EntriesPerTimeslice)[i][j] /= (int)sizeof(double);
                                }
                            }
                            else
                            {
                                (*Data)[i] = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)resp->SizeInBytes[i]);
                                memcpy((*Data)[i], payload, (size_t)((*nTimeslices)*sizeof(double)));
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc(1, (mwSize)sizeof(int));
                                (*EntriesPerTimeslice)[i][0] = -1;
                            }
                            break;

                        case MappsTypeString:
                            if ((*DataType)[i] != NULL)
                                strcpy((*DataType)[i], "string");
                            if (resp->IsVariableEncoded == 1)
                            {
                                DisconnectFromClient(h2, (EDS_Client *)client);
                                DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                                mexErrMsgTxt("Add code for case of variable string lengths per timeslice! \n");
                            }
                            else
                            {
                                if (*BytesPerEntry != NULL)
                                    (*BytesPerEntry)[i] = resp->SizeInBytes[i];  // include nul character (???)
                                elementString = (char**)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(char*));
                                for(j=0; j<*nTimeslices; j++)
                                    elementString[j] = payload+j*resp->SizeInBytes[i];
                                (*Data)[i] = (void*)mxCreateCharMatrixFromStrings((mwSize)*nTimeslices, elementString);
                                mxFree(elementString);
                                (*EntriesPerTimeslice)[i] = (int*)mxCalloc(1, (mwSize)sizeof(int));
                                (*EntriesPerTimeslice)[i][0] = -1;
                            }

                            break;

                        default:
                            if ((*DataType)[i] != NULL)
                                strcpy((*DataType)[i], "unknown");
                            (*Data)[i] = NULL;
                            break;
                    }
//                     mexPrintf("i = %d, Name = %s, Type = %s, BytesPerEntry = %d, \n", (int)i, (*Fieldnames)[i], (*DataType)[i], (*BytesPerEntry)[i]);
                }
                else {
                    if ((*DataType)[i] != NULL)
                        strcpy((*DataType)[i], "unknown");
                    if (Data[i] != NULL)    
                        (*Data)[i] = NULL;
                }
            }
        }
        
        if(Timestamps != NULL && *nTimeslices > 0)
        {
            //printf("Timestamps requested for bus = %s\n", Busname);
            if (Timestamp_DataType != NULL)
                *Timestamp_DataType = "int64";
            //printf("Timestamp data type = %s and nTimeslices = %llu\n", *Timestamp_DataType, *nTimeslices);
            *Timestamps = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int64));
            memcpy(*Timestamps, &resp->StartOfPayload, (size_t)((*nTimeslices)*sizeof(int64)));
        }
        else {
            if (Timestamp_DataType != NULL)    
                *Timestamp_DataType = "unknown";
        }
    }
	else
	{
		mexPrintf("Details (mxGetBus) for bus '%s': %s\n", Busname, ((EDS_Client *)client)->Details);
        
        *nTimeslices = 0;
        *nFields = 0;
// 		if (DataType != NULL)
//             *DataType = NULL;  
// 		if (Data != NULL)
//             *Data = NULL;
        if (Timestamp_DataType != NULL)
            *Timestamp_DataType = "unknown";
        if (Timestamps != NULL)
            *Timestamps = NULL;
	}
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    void *Buffer;
    
    char MAPPS_TIME_STRING[] = "MAPPS_time";
    
    const mxArray *FieldsRequested_ptr, *StructRequested_ptr;
    mxArray *DataStruct_ptr, *Name_ptr, *Data_ptr, *Struct_ptr;
    char *Busname, *Field_string, *MAPPS_Timestamp_DataType;
    char **Fieldnames_ptr, **MAPPS_Fieldnames_ptr, **MAPPS_DataType, *EncodedVariableFields_ptr[2];
    mwSize dims[2];
    int **MAPPS_EntriesPerTimeslice;
    uint64 *StartIndices;
    void **MAPPS_Data, *MAPPS_Timestamps;
    mwIndex i, j;
    size_t name_length, nFieldnames_total, nFieldnames;
    mwSize nBuses;
    int unique, MAPPS_nFields, *MAPPS_BytesPerEntry;
    uint64 MAPPS_nTimeslices, nTimeslices_total;
    int64 fromTime_MAPPS, toTime_MAPPS;
    mxClassID category;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    if (!(nrhs == 2 || nrhs == 3 || nrhs == 4 || nrhs == 5))
        mexErrMsgTxt("Input argument combinations allowed are [struct, struct], or [struct, struct, scalar, scalar], [struct, cellstr, string], or [struct, cellstr, string, scalar, scalar].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (nrhs == 2 || nrhs == 4) 
    {
        if (!(!(mxIsEmpty(prhs[1])) && mxIsStruct(prhs[1])))
            mexErrMsgTxt("Second input argument is not a struct.\n");
    }    
    
    if (nrhs == 3 || nrhs == 5) 
    {        
        if (!(mxIsEmpty(prhs[1]))) 
        {
            if (!(mxIsCell(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2))
                mexErrMsgTxt("Second input argument is not a cell array.\n");
            else {
                if (mxGetM(prhs[1]) != 1 && mxGetN(prhs[1]) != 1)
                    mexErrMsgTxt("Second input argument is not a vector.\n");
            }
        }
        if (!(!(mxIsEmpty(prhs[2])) && mxIsChar(prhs[2]) && mxGetNumberOfDimensions(prhs[2]) == 2 && (mxGetM(prhs[2]) == 1 || mxGetN(prhs[2]) == 1)))
            mexErrMsgTxt("Third input argument is not a string.\n");
    }
    
    if (nrhs == 4)
    {
        if (!(!(mxIsEmpty(prhs[2])) && mxIsNumeric(prhs[2]) && mxGetNumberOfDimensions(prhs[2]) == 2 && mxGetM(prhs[2]) == 1 && mxGetN(prhs[2]) == 1))
            mexErrMsgTxt("Third input argument is not a scalar.\n");
        if (!(!(mxIsEmpty(prhs[3])) && mxIsNumeric(prhs[3]) && mxGetNumberOfDimensions(prhs[3]) == 2 && mxGetM(prhs[3]) == 1 && mxGetN(prhs[3]) == 1))
            mexErrMsgTxt("Fourth input argument is not a scalar.\n");
    }
    
    if (nrhs == 5)
    {
        if (!(!(mxIsEmpty(prhs[3])) && mxIsNumeric(prhs[3]) && mxGetNumberOfDimensions(prhs[3]) == 2 && mxGetM(prhs[3]) == 1 && mxGetN(prhs[3]) == 1))
            mexErrMsgTxt("Fourth input argument is not a scalar.\n");
        if (!(!(mxIsEmpty(prhs[4])) && mxIsNumeric(prhs[4]) && mxGetNumberOfDimensions(prhs[4]) == 2 && mxGetM(prhs[4]) == 1 && mxGetN(prhs[4]) == 1))
            mexErrMsgTxt("Fifth input argument is not a scalar.\n");
    }
    
    if(nlhs != 1){
        mexErrMsgTxt("A single output argument is required.\n");
    }


    //////////////////////////////////////////////////////////////////////////
	// 2. Extract data passed from Matlab
	////////////////////////////////////////////////////////////////////////// 
    
    if (nrhs == 4) {
        fromTime_MAPPS = (int64)mxGetScalar(prhs[2]);
        toTime_MAPPS = (int64)mxGetScalar(prhs[3]);
    }
    else if (nrhs == 5) {
        fromTime_MAPPS = (int64)mxGetScalar(prhs[3]);
        toTime_MAPPS = (int64)mxGetScalar(prhs[4]);
    }
    else {
        fromTime_MAPPS = 0;
        toTime_MAPPS = 0;
    }
    
    if (nrhs == 2 || nrhs == 4) {
        StructRequested_ptr = prhs[1];
        nBuses = (mwSize)mxGetNumberOfFields(StructRequested_ptr);
        if (nBuses > 1)
            mexErrMsgIdAndTxt("MEX:GetBus:InputError", "More than a single bus requested (Second input argument).\n");
        Busname = (char *)mxGetFieldNameByNumber(StructRequested_ptr, 0);
        
        nFieldnames = (size_t)mxGetNumberOfFields(mxGetFieldByNumber(StructRequested_ptr, 0, 0));
        if (nFieldnames > 0) {
            Fieldnames_ptr = (char **)mxCalloc((mwSize)nFieldnames, sizeof(char *));
            for (i = 0; i < (mwIndex)nFieldnames; i++) {
                Field_string = (char *)mxGetFieldNameByNumber(mxGetFieldByNumber(StructRequested_ptr, 0, 0), (int)i);
                Fieldnames_ptr[i] = Field_string;
            }
        }
        else {
            nFieldnames = 0;
            Fieldnames_ptr = NULL;
        }
    }
    else {
        name_length = mxGetNumberOfElements(prhs[2]) + 1;
        Busname = (char *)mxCalloc((mwSize)name_length, sizeof(char));
        if (mxGetString(prhs[2], Busname, (mwSize)name_length) != 0)
            mexErrMsgIdAndTxt("MEX:GetBus:ConversionError", "Could not convert string data for bus name!");
                
        FieldsRequested_ptr = prhs[1];
        if (!(mxIsEmpty(FieldsRequested_ptr))) {
            nFieldnames_total = mxGetNumberOfElements(FieldsRequested_ptr);
            Fieldnames_ptr = (char **)mxCalloc((mwSize)nFieldnames_total, sizeof(char *));
            nFieldnames = 0;
            for (i = 0; i < (mwIndex)nFieldnames_total; i++) {
                Name_ptr = mxGetCell(FieldsRequested_ptr, i);
                name_length = mxGetNumberOfElements(Name_ptr) + 1;
                Field_string = (char *)mxCalloc((mwSize)name_length, sizeof(char));
                if (mxGetString(Name_ptr, Field_string, (mwSize)name_length) != 0)
                    mexErrMsgIdAndTxt("MEX:GetBus:ConversionError", "Could not convert string data for associated field name in %dth cell of input argument!", i);
                unique = 1;
                for (j = 0; j < (mwIndex)nFieldnames; j++) {
                    if (strcmp(Field_string, Fieldnames_ptr[j]) == 0) {
                        unique = 0;
                        break;
                    }
                }
                if (unique == 1) {
                    Fieldnames_ptr[i] = Field_string;
                    nFieldnames += 1;
                }
                else
                    mxFree(Field_string);
            }
            if (nFieldnames != nFieldnames_total)
                mexPrintf("NOTE: Duplicated field names in request.");
        }
        else {
            nFieldnames = 0;
            Fieldnames_ptr = NULL;
        }
    }

//     mexPrintf("Bus name = \'%s\'\n", Busname);
//     mexPrintf("Total number of fields in request = %d\n", nFieldnames);
//     for (i = 0; i < (mwIndex)nFieldnames; i++) {
//         mexPrintf("  \'%s\'\n", Fieldnames_ptr[i]);
//     }
//     mexPrintf("\n");
//     mexErrMsgTxt("E");
    
    dims[0] = 1;
    dims[1] = 1;
    DataStruct_ptr = mxCreateStructArray(2, dims, 1, (const char **)&Busname);
    Field_string = MAPPS_TIME_STRING;
    dims[0] = 1;
    dims[1] = 1;
    mxSetField(DataStruct_ptr, 0, (const char *)Busname, mxCreateStructArray(2, dims, 1, (const char **)&Field_string));
    
  
    //////////////////////////////////////////////////////////////////////////
	// 3. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
    Buffer = ConnectToClient(&h2, client);
	
	if(Buffer == NULL)
	{
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:GetBus:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		

    //////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////

    EncodedVariableFields_ptr[0] = "Data";
    EncodedVariableFields_ptr[1] = "StartIndices";

    /* 1) Search for fields "Fieldnames_ptr" in the MAPPS buffer "Busname"
     * 2) If available, copy elements between "fromTime_MAPPS" and "toTime_MAPPS" 
     *    into vector, and pass back in "MAPPS_Data"
     * 3) Report data types (single, double, uint64, char, etc.) in "MAPPS_DataType"
     * 4) If *Timestamps != NULL (i.e., associated timestamps requested), 
     *    copy MAPPS timestamps between "fromTime_MAPPS" and "toTime_MAPPS" into vector, 
     *    and pass back in "MAPPS_Timestamps"
     * 5) If *Timestamps != NULL (i.e., associated timestamps requested), 
     *    report timestamp data type (single, double, uint64, char, etc.) in "MAPPS_Timestamp_DataType"
     * 6) Report number of timeslices in "MAPPS_nTimeslices" */

    Retrieve_MAPPS_Data_Multiple_Fields((const char *)Busname, (const int)nFieldnames, (const char **)Fieldnames_ptr, fromTime_MAPPS, toTime_MAPPS,
                                        &MAPPS_nFields, &MAPPS_Fieldnames_ptr, &MAPPS_Data, &MAPPS_DataType, &MAPPS_BytesPerEntry, &MAPPS_EntriesPerTimeslice,
                                        &MAPPS_Timestamps, &MAPPS_Timestamp_DataType, &MAPPS_nTimeslices,
                                        (void *)global, (void *)client, h1, h2, Buffer);
    
//     mexPrintf("nFields = %d, nTimeslices = %d\n", MAPPS_nFields, (int)MAPPS_nTimeslices);  
//     mexPrintf("Timestamp data type = %s\n", MAPPS_Timestamp_DataType);
//     mexErrMsgTxt("E");
    if (MAPPS_Timestamps != NULL && MAPPS_nTimeslices > 0) {
        category = Interpret_MAPPS_DataType(MAPPS_Timestamp_DataType);

        dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, MAPPS_Data)
        dims[1] = 1;
        Data_ptr = mxCreateNumericArray(2, dims, category, mxREAL);
        mxSetM(Data_ptr, (mwSize)MAPPS_nTimeslices);
        mxSetData(Data_ptr, MAPPS_Timestamps);
        mxSetField(mxGetField(DataStruct_ptr, 0, (const char *)Busname), 0, MAPPS_TIME_STRING, Data_ptr);
    }
    if (MAPPS_nFields > 0  && MAPPS_nTimeslices > 0) {
        for (i = 0; i < MAPPS_nFields; i++) {
            if (MAPPS_Data[i] != NULL) {
                mxAddField(mxGetField(DataStruct_ptr, 0, (const char *)Busname), (const char *)MAPPS_Fieldnames_ptr[i]);

                category = Interpret_MAPPS_DataType(MAPPS_DataType[i]);

                if (category != mxCHAR_CLASS) {
                    if (*MAPPS_EntriesPerTimeslice[i] != -1) {                       
                        dims[0] = 1;
                        dims[1] = 1;
                        Struct_ptr = mxCreateStructArray(2, dims, 2, (const char **)EncodedVariableFields_ptr);

                        StartIndices = (uint64 *)mxCalloc((mwSize)MAPPS_nTimeslices, (mwSize)sizeof(uint64));
                        StartIndices[0] = 1;
                        nTimeslices_total = MAPPS_EntriesPerTimeslice[i][0];
                        for (j = 1; j < MAPPS_nTimeslices; j++) {
                            StartIndices[j] = 1 + nTimeslices_total;
                            nTimeslices_total += (uint64)MAPPS_EntriesPerTimeslice[i][j];
                        }
                        dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, MAPPS_Data)
                        dims[1] = 1;
                        Data_ptr = mxCreateNumericArray(2, dims, category, mxREAL);
                        mxSetM(Data_ptr, (mwSize)nTimeslices_total);
                        mxSetData(Data_ptr, MAPPS_Data[i]);
                        mxSetField(Struct_ptr, 0, (const char *)EncodedVariableFields_ptr[0], Data_ptr);
                        dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, MAPPS_Data)
                        dims[1] = 1;
                        Data_ptr = mxCreateNumericArray(2, dims, mxUINT64_CLASS, mxREAL);
                        mxSetM(Data_ptr, (mwSize)MAPPS_nTimeslices);
                        mxSetData(Data_ptr, StartIndices);
                        mxSetField(Struct_ptr, 0, (const char *)EncodedVariableFields_ptr[1], Data_ptr);

                        mxSetField(mxGetField(DataStruct_ptr, 0, (const char *)Busname), 0, (const char *)MAPPS_Fieldnames_ptr[i], Struct_ptr);
                    }
                    else {
                        dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, MAPPS_Data)
                        dims[1] = 1;
                        Data_ptr = mxCreateNumericArray(2, dims, category, mxREAL);
                        mxSetM(Data_ptr, (mwSize)MAPPS_nTimeslices);
                        mxSetData(Data_ptr, MAPPS_Data[i]);  
                        mxSetField(mxGetField(DataStruct_ptr, 0, (const char *)Busname), 0, (const char *)MAPPS_Fieldnames_ptr[i], Data_ptr);           
                    }
                }
                else {
                    mxSetField(mxGetField(DataStruct_ptr, 0, (const char *)Busname), 0, (const char *)MAPPS_Fieldnames_ptr[i], (mxArray *)MAPPS_Data[i]);                    
                }
                mxFree(MAPPS_EntriesPerTimeslice[i]);
            }
            mxFree(MAPPS_Fieldnames_ptr[i]);
            mxFree(MAPPS_DataType[i]);
        }
        mxFree(MAPPS_Fieldnames_ptr);
        mxFree(MAPPS_DataType);
        mxFree(MAPPS_BytesPerEntry);
        mxFree(MAPPS_EntriesPerTimeslice);
    }
    
    plhs[0] = DataStruct_ptr;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 5. Disconnect from the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////
	
	DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 6. Free allocated memory
	//////////////////////////////////////////////////////////////////////////
    
    if (nrhs == 3 || nrhs == 5) {
         mxFree(Busname);
        for (i = 0; i < (mwIndex)nFieldnames; i++)
            mxFree(Fieldnames_ptr[i]);
    }
    mxFree(Fieldnames_ptr);
}
