/*=================================================================
 * mxCreateBus.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void Interpret_User_DataTypeName(const char *DataTypeName, int *dataType, int *SizeInBytes)
{
    int nChars;
    char *Value_string;
    
    /*if (strcmp(DataTypeName, "uchar") == 0) {
        *dataType = ?;
        *SizeInBytes = sizeof(unsigned char);
    }
    else*/ if (strcmp(DataTypeName, "int") == 0) {
        *dataType = MappsTypeInt;
        *SizeInBytes = sizeof(int);
    }
    /*else if (strcmp(DataTypeName, "uint") == 0) {
        *dataType = MappsTypeUnknown;
        *SizeInBytes = sizeof(unsigned int);
    }*/
    else if (strcmp(DataTypeName, "int64") == 0) {
        *dataType = MappsTypeInt64;
        *SizeInBytes = sizeof(int64);
    }
    /*else if (strcmp(DataTypeName, "uint64") == 0) {
        *dataType = MappsTypeUnknown;
        *SizeInBytes = sizeof(uint64);
    }*/
    else if (strcmp(DataTypeName, "float") == 0) {
        *dataType = MappsTypeFloat;
        *SizeInBytes = sizeof(float);
    }
    else if (strcmp(DataTypeName, "double") == 0) {
        *dataType = MappsTypeDouble;
        *SizeInBytes = sizeof(double);
    }
    else if (strncmp(DataTypeName, "string", 6) == 0) {
        *dataType = MappsTypeString;
        Value_string = strtok((char *)DataTypeName, "string");
        if (Value_string == NULL)
            mexErrMsgTxt("Number of characters not specified.");
        else
            nChars = atoi(Value_string);
        if (nChars < 2)
            mexErrMsgTxt("Number of characters must be bigger than 1 (i.e., null character must be included).");
        else
            *SizeInBytes = nChars * sizeof(char);
    }
    else {
        mexErrMsgTxt("Unknown data type [accepted: 'int', 'int64', 'float', 'double', 'stringN' with N = number of characters].");
    }
}

void ExtractStringsFromArray(size_t nFields, const mxArray *Array, char **Names_ptr, int ind)
{
    size_t i, Name_length;
    mxArray *Name_ptr;
    
    for (i = 0; i < nFields; i++) {
        if (mxIsCell(Array)) {
            Name_ptr = mxGetCell(Array, (mwIndex)i);
            Name_length = mxGetNumberOfElements(Name_ptr) + 1;
            Names_ptr[i] = (char *)mxCalloc((mwSize)Name_length, sizeof(char));
            if (mxGetString(Name_ptr, Names_ptr[i], (mwSize)Name_length) != 0) {
                mexPrintf("\n\nCould not convert string data for field name in %dth cell of input argument %d.", i, ind);
                mexErrMsgTxt("Conversion error.");
            }
        }
        else if (mxIsChar(Array)) {
            Name_length = mxGetNumberOfElements(Array) + 1;
            Names_ptr[i] = (char *)mxCalloc((mwSize)Name_length, sizeof(char));
            if (mxGetString(Array, Names_ptr[i], (mwSize)Name_length) != 0) {
                mexPrintf("\n\nCould not convert string data for input argument %d.", ind);
                mexErrMsgTxt("Conversion error.");
            }
        }
        else {
            Names_ptr[i] = NULL;
        }
    }
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer;
    DS_CreateBusRequest *req;
    
    int nFrames;
    size_t i, nFields, nDataTypes, Name_length, nChars;
    int *DataType, *SizeInBytes;
    char *Busname, *Name_string;
    char **FieldNames_ptr, **DataTypeNames_ptr;
    mxArray *Data_ptr, *Field_ptr;

    
	//////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number and type of input and output arguments
	//////////////////////////////////////////////////////////////////////////     
    
    if (!(nrhs == 3 || nrhs == 5))
        mexErrMsgTxt("Input argument combinations allowed are [struct, scalar, struct with single non-empty field] or [struct, scalar, string, string/cellstr, string/cellstr].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");

    if (!(!(mxIsEmpty(prhs[1])) && mxIsNumeric(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2 && mxGetM(prhs[1]) == 1 && mxGetN(prhs[1]) == 1 && mxGetScalar(prhs[1]) > 0))
        mexErrMsgTxt("Second input argument is not a positive scalar.\n");
    
    if (nrhs == 3) {
        if (!(!(mxIsEmpty(prhs[2])) && mxIsStruct(prhs[2]) &&  mxGetNumberOfFields(prhs[2]) == 1))
            mexErrMsgTxt("Third input argument is not a struct with single non-empty field.\n");

        nFields = (size_t)mxGetNumberOfFields(mxGetFieldByNumber(prhs[2], 0, 0));
        nDataTypes = nFields;
    }
    if (nrhs == 5) {
        if (!(!(mxIsEmpty(prhs[2])) && mxIsChar(prhs[2]) && mxGetNumberOfDimensions(prhs[2]) == 2 && (mxGetM(prhs[2]) == 1 || mxGetN(prhs[2]) == 1)))
            mexErrMsgTxt("Third input argument is not a string.\n");
        
        if (!(!(mxIsEmpty(prhs[3])) && (mxIsChar(prhs[3]) || mxIsCell(prhs[3])) && mxGetNumberOfDimensions(prhs[3]) == 2 && (mxGetM(prhs[3]) == 1 || mxGetN(prhs[3]) == 1)))
            mexErrMsgTxt("Fourth input argument is not a string or cellstr.\n");
        
        if (!(!(mxIsEmpty(prhs[4])) && (mxIsChar(prhs[4]) || mxIsCell(prhs[4])) && mxGetNumberOfDimensions(prhs[4]) == 2 && (mxGetM(prhs[4]) == 1 || mxGetN(prhs[4]) == 1)))
            mexErrMsgTxt("Fifth input argument is not a string or cellstr.\n");
        
        if (mxIsCell(prhs[3])) 
        {
            nFields = mxGetM(prhs[3]);
            if (nFields == 1)
                nFields = mxGetN(prhs[3]);            
        }    
        else 
            nFields = 1;
        
        if (mxIsCell(prhs[4])) 
        {
            nDataTypes = mxGetM(prhs[4]);           
            if (nDataTypes == 1)
                nDataTypes = mxGetN(prhs[4]);
        }
        else 
            nDataTypes = 1;
        
        if (nDataTypes != nFields)
            mexErrMsgTxt("Mismatch in number of entries between fourth and fifth arguments.\n");
    }
        
    if (nlhs != 0){
        mexErrMsgTxt("No output argument is required.\n");
    }

    
	//////////////////////////////////////////////////////////////////////////
	// 2. Extract data from input arguments
	//////////////////////////////////////////////////////////////////////////     
    
    nFrames = (int)mxGetScalar(prhs[1]);
    FieldNames_ptr = (char **)mxCalloc((mwSize)nFields, sizeof(char *));
    DataType = (int *)mxCalloc((mwSize)nFields, sizeof(int));
    SizeInBytes = (int *)mxCalloc((mwSize)nFields, sizeof(int)); 
    if (nrhs == 3) {
        Busname = (char *)mxGetFieldNameByNumber(prhs[2], 0);
        Data_ptr = mxGetField(prhs[2], 0, (const char *)Busname);
        for (i = 0; i < nFields; i++) {
            Name_string = (char *)mxGetFieldNameByNumber(Data_ptr, (int)i);
            Name_length = strlen(Name_string) + 1;
            FieldNames_ptr[i] = (char *)mxCalloc((mwSize)Name_length, sizeof(char));
            strcpy(FieldNames_ptr[i], Name_string);
            Field_ptr = mxGetFieldByNumber(Data_ptr, 0, (int)i);
            switch (mxGetClassID(Field_ptr)) {
                /*case mxUINT8_CLASS:
                    DataType[i] = ?;
                    SizeInBytes[i] = sizeof(unsigned char);
                    break;*/
                case mxINT32_CLASS:
                    DataType[i] = MappsTypeInt;
                    SizeInBytes[i] = sizeof(int);
                    break;
                /*case mxUINT32_CLASS:
                    DataType[i] = ?;
                    SizeInBytes[i] = sizeof(unsigned int);
                    break;*/
                case mxINT64_CLASS:
                    DataType[i] = MappsTypeInt64;
                    SizeInBytes[i] = sizeof(int64);
                    break;
                /*case mxUINT64_CLASS:
                    DataType[i] = ?;
                    SizeInBytes[i] = sizeof(uint64);
                    break;*/
                case mxSINGLE_CLASS:
                    DataType[i] = MappsTypeFloat;
                    SizeInBytes[i] = sizeof(float);
                    break;
                case mxDOUBLE_CLASS:
                    DataType[i] = MappsTypeDouble;
                    SizeInBytes[i] = sizeof(double);
                    break;
                case mxCHAR_CLASS:
                    DataType[i] = MappsTypeString;
                    nChars = mxGetM(Field_ptr);
                    if (nChars == 1 || nChars == nFrames)
                        nChars = mxGetN(Field_ptr);
                    SizeInBytes[i] = (int)((nChars + 1) * sizeof(char));
                    break;
                default:
                    mexErrMsgTxt("Unhandled type of class.\n");
                    break;
            }     
        }    
    }
    else {
        DataTypeNames_ptr = (char **)mxCalloc((mwSize)nFields, sizeof(char *));
        Name_length = mxGetNumberOfElements(prhs[2]) + 1;
        Busname = (char *)mxCalloc((mwSize)Name_length, sizeof(char));
        if (mxGetString(prhs[2], Busname, (mwSize)Name_length) != 0) {
            mexPrintf("\n\nCould not convert string data for third input argument.");
            mexErrMsgTxt("Conversion error.");
        }
        ExtractStringsFromArray(nFields, prhs[3], FieldNames_ptr, 3);
        ExtractStringsFromArray(nFields, prhs[4], DataTypeNames_ptr, 4);
        for (i = 0; i < nFields; i++) {
            Interpret_User_DataTypeName((const char *)DataTypeNames_ptr[i], &(DataType[i]), &(SizeInBytes[i]));
        }
    }
    
    /*mexPrintf("Bus: %s; nFrames = %d; nFields = %d\n", Busname, nFrames, (int)nFields);
    for (i = 0; i < nFields; i++) {       
        mexPrintf("  %s: -type: %d  -size: %d\n", FieldNames_ptr[i], DataType[i], SizeInBytes[i]);
    }*/
    
    
    //////////////////////////////////////////////////////////////////////////
	// 3. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
    Buffer = ConnectToClient(&h2, client);
	
	if(Buffer == NULL)
	{
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:CreateBus:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}


	//////////////////////////////////////////////////////////////////////////
	// 4. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_CreateBus) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:CreateBus:ClientBlocked", "Server is still blocking the client channel! \n");
	}
	
    
	//////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////
	
	req = (DS_CreateBusRequest *)Buffer;
    bufferSize = 0;
    
    req->FrameCount = nFrames;
    sprintf(req->BusName, Busname);
    req->ElementCount = (int)nFields;
    for (i = 0; i < nFields; i++) {       
        sprintf(req->Elements[i].ElementName, FieldNames_ptr[i]);
        req->Elements[i].Type = DataType[i];
        req->Elements[i].SizeInBytes = SizeInBytes[i];
    }
    
    bufferSize += sizeof(DS_CreateBusRequest);
    
    
	//////////////////////////////////////////////////////////////////////////
	// 5. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:CreateBus:ServerTimedOut", "SERVER TIMED OUT! \n");
    }
	
	
    //////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////
    
    if(strncmp(client->Result, "success", 7) != 0)
	    mexPrintf("Details (mxCreateBus): %s\n", client->Details);
    
    
	//////////////////////////////////////////////////////////////////////////
	// 7. Disconnect from the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////
	
	DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
	

	//////////////////////////////////////////////////////////////////////////
	// 8. Free allocated memory
	//////////////////////////////////////////////////////////////////////////
    
    if (nrhs == 5) {
        mxFree(Busname);
        for (i = 0; i < nFields; i++) {
            mxFree(DataTypeNames_ptr[i]);
        }
        mxFree(DataTypeNames_ptr);
    }
    
    for (i = 0; i < nFields; i++) {
        mxFree(FieldNames_ptr[i]);
    }
    mxFree(FieldNames_ptr);
    mxFree(DataType);
    mxFree(SizeInBytes);
}
