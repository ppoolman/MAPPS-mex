/*=================================================================
 * mxUpdateBus.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void Populate_Shared_Memory_Buffer(
    const void *Buffer,
    size_t *payloadSize,
	const char *Busname,
    const uint64 nTimestamps,
    const void *Timestamps, 
	const mxClassID Timestamp_Type,    
    const int nElements,
    const char **ElementNames_ptr,
    const int *MAPPSElement_Type,
    const int *MAPPSElement_SizeInBytes,
    const int *MAPPSOrderOfFields,
    const int nFields,
	const char **FieldNames_ptr,  
	const void **FieldData_ptr, 
	const mxClassID *Field_Type,
    const int *Field_SizeInBytes,
	void *global, void *client, void *h1, void *h2)
{
    
    /* Field_SizeInBytes[i]: number of bytes per timeslice (usually = sizeof(element)), 
     *              but for strings, it is the number of bytes (characters) WITHOUT the null character ("\0")
     * FieldData_ptr[i]: for strings, there is no null character at the end of a string for each timeslice */  
    
    char *FieldData, *MAPPS_Data, *String;
    int64 *MAPPS_Timestamps;
    int i, fieldID, showTypeWarning;
    uint64 j;
    size_t bytes_MAPPSData, bytes_FieldData, bytes_Copy;
    
    unsigned char *UInt8_Matlab;
    int *Int32_Matlab;
    unsigned int *UInt32_Matlab;    
    int64 *Int64_Matlab;
    uint64 *UInt64_Matlab;
    float *Float_Matlab;
    double *Double_Matlab;
    
    int *Int_MAPPS;
    int64 *Int64_MAPPS;
    float *Float_MAPPS;
    double *Double_MAPPS;
    
    unsigned long raw_f = 0x7fc00000;
    unsigned long long raw_d = 0x7ff8000000000000;
    
    *payloadSize = 0;

    mexPrintf("==========================================================\n");
    mexPrintf("Name of bus = %s\n", Busname);
    mexPrintf("Number of timestamps = %llu\n", nTimestamps);
    switch (Timestamp_Type) {
        case mxUINT8_CLASS:
            mexPrintf("uchar: First timestamp = %u\n", *((unsigned char *)Timestamps));
            break;
        case mxINT32_CLASS:
            mexPrintf("int: First timestamp = %d\n", *((int *)Timestamps));
            break;
        case mxUINT32_CLASS:
            mexPrintf("uint: First timestamp = %u\n", *((unsigned int *)Timestamps));
            break;
        case mxINT64_CLASS:
            mexPrintf("int64: First timestamp = %lld\n", *((int64 *)Timestamps));
            break;
        case mxUINT64_CLASS:
            mexPrintf("uint64: First timestamp = %llu\n", *((uint64 *)Timestamps));
            break;
        case mxSINGLE_CLASS:
            mexPrintf("float: First timestamp = %f\n", *((float *)Timestamps));
            break;
        case mxDOUBLE_CLASS:
            mexPrintf("double: First timestamp = %f\n", *((double *)Timestamps));
            break;
        default:
            mexPrintf("Incorrect type for timestamps\n");
            break;
    }
    for (i = 0; i < nFields; i++) {
        mexPrintf("----------------------------------------------------------\n");
        mexPrintf("Name of Matlab field %d = %s.  Bytes per entry = %d\n", i + 1, FieldNames_ptr[i], Field_SizeInBytes[i]);
        FieldData = (char *)FieldData_ptr[i];
        switch (Field_Type[i]) {
            case mxUINT8_CLASS:
                mexPrintf("  uchar: First value = %u\n", *((unsigned char *)FieldData));
                break;
            case mxINT32_CLASS:
                mexPrintf("  int: First value = %d\n", *((int *)FieldData));
                break;
            case mxUINT32_CLASS:
                mexPrintf("  uint: First value = %u\n", *((unsigned int *)FieldData));
                break;
            case mxINT64_CLASS:
                mexPrintf("  int64: First value = %lld\n", *((int64 *)FieldData));
                break;
            case mxUINT64_CLASS:
                mexPrintf("  uint64: First value = %llu\n", *((uint64 *)FieldData));
                break;
            case mxSINGLE_CLASS:
                mexPrintf("  float: First value = %f\n", *((float *)FieldData));
                break;
            case mxDOUBLE_CLASS:
                mexPrintf("  double: First value = %f\n", *((double *)FieldData));
                break;
            case mxCHAR_CLASS:
                mexPrintf("  string: First character = %c\n", *((char *)FieldData));
                break;
            default:
                mexPrintf("  Incorrect type for field %s\n", FieldNames_ptr[i]);
                break;
        }        
    }
    mexPrintf("==========================================================\n");
    
    MAPPS_Timestamps = (int64 *)Buffer;
    showTypeWarning = 1;
    switch (Timestamp_Type) {
        case mxUINT8_CLASS:
            UInt8_Matlab = (unsigned char *)Timestamps;
            for (j = 0; j < nTimestamps; j++)
                *MAPPS_Timestamps++ = (int64)*UInt8_Matlab++;
            break;
        case mxINT32_CLASS:
            Int32_Matlab = (int *)Timestamps;
            for (j = 0; j < nTimestamps; j++)
                *MAPPS_Timestamps++ = (int64)*Int32_Matlab++;
            break;
        case mxUINT32_CLASS:
            UInt32_Matlab = (unsigned int *)Timestamps;
            for (j = 0; j < nTimestamps; j++)
                *MAPPS_Timestamps++ = (int64)*UInt32_Matlab++;
            break;
        case mxINT64_CLASS:
            memcpy((void *)MAPPS_Timestamps, Timestamps, (size_t)(nTimestamps) * sizeof(int64));
            showTypeWarning = 0;
            break;
        case mxUINT64_CLASS:
            UInt64_Matlab = (uint64 *)Timestamps;
            for (j = 0; j < nTimestamps; j++)
                *MAPPS_Timestamps++ = (int64)*UInt64_Matlab++;
            break;
        case mxSINGLE_CLASS:
            Float_Matlab = (float *)Timestamps;
            for (j = 0; j < nTimestamps; j++)
                *MAPPS_Timestamps++ = (int64)*Float_Matlab++;
            break;
        case mxDOUBLE_CLASS:
            Double_Matlab = (double *)Timestamps;
            for (j = 0; j < nTimestamps; j++)
                *MAPPS_Timestamps++ = (int64)*Double_Matlab++;
            break;
        default:
            DisconnectFromClient(h2, (EDS_Client *)client);
            DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
            mexErrMsgIdAndTxt("MEX:UpdateBus:UnhandledType", "Unhandled type for timestamps!");
            break;
    }
    if (showTypeWarning == 1)
        mexPrintf("Data transfer of timestamps might be faster if passed as 'int64' from Matlab. \n");
    MAPPS_Data = (char *)Buffer + nTimestamps * sizeof(int64);
    *payloadSize += (size_t)(nTimestamps) * sizeof(int64);
   
    for (i = 0; i < nElements; i++) {
        fieldID = MAPPSOrderOfFields[i];
        if (fieldID < 0) {
            switch (MAPPSElement_Type[i]) {
                case MappsTypeInt:
                    mexPrintf("No Matlab data provided for field %s - passing zeros... \n", ElementNames_ptr[i]);
                    Int_MAPPS = (int *)MAPPS_Data;
                    for (j = 0; j < nTimestamps; j++)
                        *Int_MAPPS++ = 0;
                    MAPPS_Data += (size_t)(nTimestamps) * sizeof(int);
                    *payloadSize += (size_t)(nTimestamps) * sizeof(int);
                    break;
                case MappsTypeInt64:
                    mexPrintf("No Matlab data provided for field %s - passing zeros... \n", ElementNames_ptr[i]);
                    Int64_MAPPS = (int64 *)MAPPS_Data;
                    for (j = 0; j < nTimestamps; j++)
                        *Int64_MAPPS++ = 0;
                    MAPPS_Data += (size_t)(nTimestamps) * sizeof(int64);
                    *payloadSize += (size_t)(nTimestamps) * sizeof(int64);
                    break;
                case MappsTypeFloat:
                    mexPrintf("No Matlab data provided for field %s - passing NaNs... \n", ElementNames_ptr[i]);
                    Float_MAPPS = (float *)MAPPS_Data;
                    for (j = 0; j < nTimestamps; j++)
                        *Float_MAPPS++ = *(float *)&raw_f;
                    MAPPS_Data += (size_t)(nTimestamps) * sizeof(float);
                    *payloadSize += (size_t)(nTimestamps) * sizeof(float);
                    break;
                case MappsTypeDouble:
                    mexPrintf("No Matlab data provided for field %s - passing NaNs... \n", ElementNames_ptr[i]);
                    Double_MAPPS = (double *)MAPPS_Data;
                    for (j = 0; j < nTimestamps; j++)
                        *Double_MAPPS++ = *(double *)&raw_d;
                    MAPPS_Data += (size_t)(nTimestamps) * sizeof(double);
                    *payloadSize += (size_t)(nTimestamps) * sizeof(double);
                    break;
                case MappsTypeString:
                    mexPrintf("No Matlab data provided for field %s - passing empty strings... \n", ElementNames_ptr[i]);
                    memset(MAPPS_Data, 0, (size_t)nTimestamps * (size_t)MAPPSElement_SizeInBytes[i]);   // ???? - any null \0 characters needed?
                    MAPPS_Data += (size_t)(nTimestamps) * (size_t)MAPPSElement_SizeInBytes[i];
                    *payloadSize += (size_t)(nTimestamps) * (size_t)MAPPSElement_SizeInBytes[i];
                    break;
                case MappsTypeUnknown:
                    DisconnectFromClient(h2, (EDS_Client *)client);
                    DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                    mexErrMsgIdAndTxt("MEX:UpdateBus:UnknownType", "Type reported as 'unknown' by server for field %s (No Matlab data provided for this field)!", ElementNames_ptr[i]);
                    break;
                default:
                    DisconnectFromClient(h2, (EDS_Client *)client);
                    DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                    mexErrMsgIdAndTxt("MEX:UpdateBus:NoType", "No or unrecognized type reported by server for field %s (No Matlab data provided for this field). \n", ElementNames_ptr[i]);
                    break;
            }
        }
        else {
            switch (MAPPSElement_Type[i]) {
                case MappsTypeInt:
                    Int_MAPPS = (int *)MAPPS_Data;
                    showTypeWarning = 1;
                    switch (Field_Type[fieldID]) {
                        case mxUINT8_CLASS:
                            UInt8_Matlab = (unsigned char *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int_MAPPS++ = (int)*UInt8_Matlab++;
                            break;
                        case mxINT32_CLASS:
                            memcpy((void *)MAPPS_Data, (void *)((char **)FieldData_ptr)[fieldID], (size_t)(nTimestamps) * sizeof(int));
                            showTypeWarning = 0;
                            break;
                        case mxUINT32_CLASS:
                            UInt32_Matlab = (unsigned int *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int_MAPPS++ = (int)*UInt32_Matlab++;
                            break;
                        case mxINT64_CLASS:
                            Int64_Matlab = (int64 *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int_MAPPS++ = (int)*Int64_Matlab++;
                            break;
                        case mxUINT64_CLASS:
                            UInt64_Matlab = (uint64 *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int_MAPPS++ = (int)*UInt64_Matlab++;
                            break;
                        case mxSINGLE_CLASS:
                            Float_Matlab = (float *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int_MAPPS++ = (int)*Float_Matlab++;
                            break;
                        case mxDOUBLE_CLASS:
                            Double_Matlab = (double *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int_MAPPS++ = (int)*Double_Matlab++;
                            break;
                        default:
                            mexPrintf("Unhandled Matlab type for field %s - passing zeros... \n", FieldNames_ptr[fieldID]);
                            for (j = 0; j < nTimestamps; j++)
                                *Int_MAPPS++ = 0;
                            break;
                    }
                    if (showTypeWarning == 1)
                        mexPrintf("Data transfer for field %s might be faster if passed as 'int' from Matlab. \n", FieldNames_ptr[fieldID]);
                    MAPPS_Data += (size_t)(nTimestamps) * sizeof(int);
                    *payloadSize += (size_t)(nTimestamps) * sizeof(int);
                    break;
                case MappsTypeInt64:
                    Int64_MAPPS = (int64 *)MAPPS_Data;
                    showTypeWarning = 1;
                    switch (Field_Type[fieldID]) {
                        case mxUINT8_CLASS:
                            UInt8_Matlab = (unsigned char *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int64_MAPPS++ = (int64)*UInt8_Matlab++;
                            break;
                        case mxINT32_CLASS:
                            Int32_Matlab = (int *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int64_MAPPS++ = (int64)*Int32_Matlab++;
                            break;
                        case mxUINT32_CLASS:
                            UInt32_Matlab = (unsigned int *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int64_MAPPS++ = (int64)*UInt32_Matlab++;
                            break;
                        case mxINT64_CLASS:
                            memcpy((void *)MAPPS_Data, (void *)((char **)FieldData_ptr)[fieldID], (size_t)(nTimestamps) * sizeof(int64));
                            showTypeWarning = 0;
                            break;
                        case mxUINT64_CLASS:
                            UInt64_Matlab = (uint64 *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int64_MAPPS++ = (int64)*UInt64_Matlab++;
                            break;
                        case mxSINGLE_CLASS:
                            Float_Matlab = (float *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int64_MAPPS++ = (int64)*Float_Matlab++;
                            break;
                        case mxDOUBLE_CLASS:
                            Double_Matlab = (double *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Int64_MAPPS++ = (int64)*Double_Matlab++;
                            break;
                        default:
                            mexPrintf("Unhandled Matlab type for field %s - passing zeros... \n", FieldNames_ptr[fieldID]);
                            for (j = 0; j < nTimestamps; j++)
                                *Int64_MAPPS++ = 0;
                            break;
                    }
                    if (showTypeWarning == 1)
                        mexPrintf("Data transfer for field %s might be faster if passed as 'int64' from Matlab. \n", FieldNames_ptr[fieldID]);
                    MAPPS_Data += (size_t)(nTimestamps) * sizeof(int64);
                    *payloadSize += (size_t)(nTimestamps) * sizeof(int64);
                    break;
                case MappsTypeFloat:
                    Float_MAPPS = (float *)MAPPS_Data;
                    showTypeWarning = 1;
                    switch (Field_Type[fieldID]) {
                        case mxUINT8_CLASS:
                            UInt8_Matlab = (unsigned char *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Float_MAPPS++ = (float)*UInt8_Matlab++;
                            break;
                        case mxINT32_CLASS:
                            Int32_Matlab = (int *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Float_MAPPS++ = (float)*Int32_Matlab++;
                            break;
                        case mxUINT32_CLASS:
                            UInt32_Matlab = (unsigned int *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Float_MAPPS++ = (float)*UInt32_Matlab++;
                            break;
                        case mxINT64_CLASS:
                            Int64_Matlab = (int64 *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Float_MAPPS++ = (float)*Int64_Matlab++;
                            break;
                        case mxUINT64_CLASS:
                            UInt64_Matlab = (uint64 *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Float_MAPPS++ = (float)*UInt64_Matlab++;
                            break;
                        case mxSINGLE_CLASS:
                            memcpy((void *)MAPPS_Data, (void *)((char **)FieldData_ptr)[fieldID], (size_t)(nTimestamps) * sizeof(float));
                            showTypeWarning = 0;
                            break;
                        case mxDOUBLE_CLASS:
                            Double_Matlab = (double *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Float_MAPPS++ = (float)*Double_Matlab++;
                            break;
                        default:
                            mexPrintf("Unhandled Matlab type for field %s - passing NaNs... \n", FieldNames_ptr[fieldID]);
                            for (j = 0; j < nTimestamps; j++)
                                *Float_MAPPS++ = *(float *)&raw_f;
                            break;
                    }
                    if (showTypeWarning == 1)
                        mexPrintf("Data transfer for field %s might be faster if passed as 'single' from Matlab. \n", FieldNames_ptr[fieldID]);
                    MAPPS_Data += (size_t)(nTimestamps) * sizeof(float);
                    *payloadSize += (size_t)(nTimestamps) * sizeof(float);
                    break;
                case MappsTypeDouble:
                    Double_MAPPS = (double *)MAPPS_Data;
                    showTypeWarning = 1;
                    switch (Field_Type[fieldID]) {
                        case mxUINT8_CLASS:
                            UInt8_Matlab = (unsigned char *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Double_MAPPS++ = (double)*UInt8_Matlab++;
                            break;
                        case mxINT32_CLASS:
                            Int32_Matlab = (int *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Double_MAPPS++ = (double)*Int32_Matlab++;
                            break;
                        case mxUINT32_CLASS:
                            UInt32_Matlab = (unsigned int *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Double_MAPPS++ = (double)*UInt32_Matlab++;
                            break;
                        case mxINT64_CLASS:
                            Int64_Matlab = (int64 *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Double_MAPPS++ = (double)*Int64_Matlab++;
                            break;
                        case mxUINT64_CLASS:
                            UInt64_Matlab = (uint64 *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Double_MAPPS++ = (double)*UInt64_Matlab++;
                            break;
                        case mxSINGLE_CLASS:
                            Float_Matlab = (float *)(((char **)FieldData_ptr)[fieldID]); 
                            for (j = 0; j < nTimestamps; j++)
                                *Double_MAPPS++ = (double)*Float_Matlab++;
                            break;
                        case mxDOUBLE_CLASS:
                            memcpy((void *)MAPPS_Data, (void *)((char **)FieldData_ptr)[fieldID], (size_t)(nTimestamps) * sizeof(double));
                            showTypeWarning = 0;
                            break;
                        default:
                            mexPrintf("Unhandled Matlab type for field %s - passing NaNs... \n", FieldNames_ptr[fieldID]);
                            for (j = 0; j < nTimestamps; j++)
                                *Double_MAPPS++ = *(double *)&raw_d;
                            break;
                    }
                    if (showTypeWarning == 1)
                        mexPrintf("Data transfer for field %s might be faster if passed as 'double' from Matlab. \n", FieldNames_ptr[fieldID]);
                    MAPPS_Data += (size_t)(nTimestamps) * sizeof(double);
                    *payloadSize += (size_t)(nTimestamps) * sizeof(double);
                    break;
                case MappsTypeString:
                    if (Field_Type[fieldID] == mxCHAR_CLASS) {    
                        String = ((char **)FieldData_ptr)[fieldID];
                        bytes_FieldData = (size_t)Field_SizeInBytes[fieldID];
                        bytes_MAPPSData = (size_t)MAPPSElement_SizeInBytes[i] - 1;
                        memset(MAPPS_Data, 0, (size_t)nTimestamps * (bytes_MAPPSData + 1));
                        if (bytes_FieldData > bytes_MAPPSData) {
                            mexPrintf("Truncating each entry to %llu characters for field %s (null character excluded)... \n", (uint64)bytes_MAPPSData, FieldNames_ptr[fieldID]);
                            bytes_Copy = bytes_MAPPSData;
                        }
                        else
                            bytes_Copy = bytes_FieldData;
                        for (j = 0; j < nTimestamps; j++) {
                            memcpy((void *)MAPPS_Data, (const void *)String, bytes_Copy);
                            MAPPS_Data += bytes_MAPPSData + 1;
                            *payloadSize += bytes_MAPPSData + 1;
                            String += bytes_FieldData;
                        }
                    }
                    else {
                        DisconnectFromClient(h2, (EDS_Client *)client);
                        DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                        mexErrMsgIdAndTxt("MEX:UpdateBus:NotStringType", "Data provided for field %s are not of type 'string'. \n", FieldNames_ptr[fieldID]);
                    }
                    break;
                case MappsTypeUnknown:
                    DisconnectFromClient(h2, (EDS_Client *)client);
                    DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                    mexErrMsgIdAndTxt("MEX:UpdateBus:UnknownType", "Type reported as 'unknown' by server for field %s. \n", FieldNames_ptr[fieldID]);
                    break;
                default:
                    DisconnectFromClient(h2, (EDS_Client *)client);
                    DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                    mexErrMsgIdAndTxt("MEX:UpdateBus:NoType", "No or unrecognized type reported by server for field %s. \n", FieldNames_ptr[fieldID]);
                    break;
            }
        }        
    }
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    void *Buffer;
    DS_ListBusResponse *listResp;
    DS_UpdateBusRequest *req;
    
	const mxArray *Timestamps_ptr, *Struct_ptr, *Data_ptr, *Field_ptr;
    mxArray *CharMatrix;
    char *Timestamps, *Field_string, *Bus_string;
    mwSize dims[2];
    char **FieldNames_ptr, **MAPPSElement_Names_ptr;
    void **FieldData_ptr;
    int error_Type, busID, ignoreField;
    int *Field_SizeInBytes, *MAPPSElement_Type, *MAPPSElement_SizeInBytes, *MAPPSOrderOfFields;
    size_t nFields, nEntries, Name_length, m, n, bufferSize, payloadSize;
    uint64 nTimestamps;
    mwIndex i, j, nElements;
    mxClassID Timestamp_Type;
    mxClassID *Field_Type;

    
    //////////////////////////////////////////////////////////////////////////
	// 0. Initialize variables
	////////////////////////////////////////////////////////////////////////// 
    
    FieldNames_ptr = NULL;
    FieldData_ptr = NULL;
    Field_Type = NULL;
    Field_SizeInBytes = NULL;
    MAPPSElement_Names_ptr = NULL;
    MAPPSElement_Type = NULL;
    MAPPSElement_SizeInBytes = NULL;
    MAPPSOrderOfFields = NULL;

    
	//////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number and type of input and output arguments
	////////////////////////////////////////////////////////////////////////// 
    
    /*  prhs[1] <-- vector of MAPPS-type timestamps
     *  prhs[2] <-- structure containing data to be passed to MAPPS 
     
     *  plhs[0] <-- error code */
    
    if (!(nrhs == 3))
        mexErrMsgTxt("Input argument combination allowed is [struct, numeric vector, struct with single non-empty field].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (!(!(mxIsEmpty(prhs[1])) && mxIsNumeric(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2 && (mxGetM(prhs[1]) == 1 || mxGetN(prhs[1]) == 1)))
        mexErrMsgTxt("Second input argument is not a numeric vector.\n");
    
    if (!(!(mxIsEmpty(prhs[2])) && mxIsStruct(prhs[2]) &&  mxGetNumberOfFields(prhs[2]) == 1))
        mexErrMsgTxt("Third input argument is not a struct with single non-empty field.\n");
           
    if (!(nlhs == 0 || nlhs == 1))
        mexErrMsgTxt("No more than a single output argument is required.");
        
    Timestamps_ptr = prhs[1];
    Struct_ptr = prhs[2];
    
    nTimestamps = (uint64)mxGetNumberOfElements(Timestamps_ptr);
    Timestamps = (char *)mxGetData(Timestamps_ptr);
    Timestamp_Type = mxGetClassID(Timestamps_ptr);
    Bus_string = (char *)mxGetFieldNameByNumber(Struct_ptr, 0);
    Data_ptr = mxGetFieldByNumber(Struct_ptr, 0, 0);
    if (!((!(mxIsEmpty(Data_ptr))) && mxIsStruct(Data_ptr))) {
        mexPrintf("\nAt least a single non-empty field is required for bus = %s. \n", Bus_string);
        mexErrMsgTxt("No contents provided for bus.");
    }
    nFields = (size_t)mxGetNumberOfFields(Data_ptr);
    error_Type = 0;
    for (i = 0; i < (mwIndex)nFields; i++) {
        Field_ptr = mxGetFieldByNumber(Data_ptr, 0, (int)i);
        if (!(mxIsNumeric(Field_ptr) || mxIsChar(Field_ptr))) {
            error_Type = 1;
            if (mxGetClassID(Field_ptr) == mxCELL_CLASS)
                mexPrintf("\nField %llu is a cell array. If applicable, call 'char' to convert a cell array of strings into a char array first.\n", (uint64)i + 1);
            else
                mexPrintf("\nField %llu: type not supported");  
        }        
    }
    if (error_Type == 1)
        mexErrMsgTxt("Type(s) of one or more fields not supported.");
    
    for (i = 0; i < (mwIndex)nFields; i++) {
        Field_ptr = mxGetFieldByNumber(Data_ptr, 0, (int)i);
        nEntries = mxGetNumberOfElements(Field_ptr);
        if (mxIsChar(Field_ptr)) {
            if (!(mxGetNumberOfDimensions(Field_ptr) == 2 && (mxGetM(Field_ptr) == nTimestamps || mxGetN(Field_ptr) == nTimestamps))) {
                mexPrintf("\nSize of char array in field %llu = [%llu x %llu].  Number of timestamps = %llu. \n", (uint64)i + 1, (uint64)mxGetM(Field_ptr), (uint64)mxGetN(Field_ptr), (uint64)nTimestamps);
                mexErrMsgTxt("Inconsistent number of elements.");
            }
        }
        else if (!(nEntries == nTimestamps && mxGetNumberOfDimensions(Field_ptr) == 2 && (mxGetM(Field_ptr) == 1 || mxGetN(Field_ptr) == 1))) {
            mexPrintf("\nNumber of elements for field %llu = %llu.  Number of timestamps = %llu. \n", (uint64)i + 1, (uint64)nEntries, (uint64)nTimestamps);
            mexErrMsgTxt("Inconsistent number of elements.");
        }
    }
    
    
	//////////////////////////////////////////////////////////////////////////
	// 2. Extract data passed from Matlab
	////////////////////////////////////////////////////////////////////////// 
      
    FieldNames_ptr = (char **)mxCalloc((mwSize)nFields, sizeof(char *));
    FieldData_ptr = (void **)mxCalloc((mwSize)nFields, sizeof(void *));
    Field_Type = (mxClassID *)mxCalloc((mwSize)nFields, sizeof(mxClassID));
    Field_SizeInBytes = (int *)mxCalloc((mwSize)nFields, sizeof(int));
    for (i = 0; i < (mwIndex)nFields; i++) {
        Field_string = (char *)mxGetFieldNameByNumber(Data_ptr, (int)i);    // Matlab struct ensures that fields can't share the same name
        Name_length = strlen(Field_string) + 1;
        FieldNames_ptr[i] = (char *)mxCalloc((mwSize)Name_length, sizeof(char));
        strcpy(FieldNames_ptr[i], Field_string);
        
        Field_ptr = mxGetFieldByNumber(Data_ptr, 0, (int)i);
        Field_Type[i] = mxGetClassID(Field_ptr);
        if (Field_Type[i] == mxCHAR_CLASS) {
            m = mxGetM(Field_ptr);
            n = mxGetN(Field_ptr);
            if (m == (size_t)nTimestamps) {
                mexCallMATLAB(1, &CharMatrix, 1, (mxArray **)&Field_ptr, "transpose");
                // FieldData_ptr[i] = mxArrayToString(CharMatrix);  // Doesn't work????? single vs. multi-byte character set?
                FieldData_ptr[i] = (void *)mxCalloc((mwSize)(n * (size_t)nTimestamps + 1), sizeof(char));
                error_Type = mxGetString(CharMatrix, (char *)FieldData_ptr[i], (mwSize)(n * (size_t)nTimestamps + 1));
                mxDestroyArray(CharMatrix);
                Field_SizeInBytes[i] = (int)(n * sizeof(char));
            }
            else {
                // FieldData_ptr[i] = mxArrayToString(Field_ptr);   // Doesn't work????? single vs. multi-byte character set?
                FieldData_ptr[i] = (void *)mxCalloc((mwSize)(m * (size_t)nTimestamps + 1), sizeof(char));
                error_Type = mxGetString(Field_ptr, (char *)FieldData_ptr[i], (mwSize)(m * (size_t)nTimestamps + 1));
                Field_SizeInBytes[i] = (int)(m * sizeof(char));
            }
        }
        else {
            FieldData_ptr[i] = (void *)mxGetData(Field_ptr);
            switch (Field_Type[i]) {
                case mxUINT8_CLASS:
                    Field_SizeInBytes[i] = (int)sizeof(unsigned char);
                    break;
                case mxINT32_CLASS:
                    Field_SizeInBytes[i] = (int)sizeof(int);
                    break;
                case mxUINT32_CLASS:
                    Field_SizeInBytes[i] = (int)sizeof(unsigned int);
                    break;
                case mxINT64_CLASS:
                    Field_SizeInBytes[i] = (int)sizeof(int64);
                    break;
                case mxUINT64_CLASS:
                    Field_SizeInBytes[i] = (int)sizeof(uint64);
                    break;
                case mxSINGLE_CLASS:
                    Field_SizeInBytes[i] = (int)sizeof(float);
                    break;
                case mxDOUBLE_CLASS:
                    Field_SizeInBytes[i] = (int)sizeof(double);
                    break;
                default:
                    mexErrMsgTxt("Unhandled type of class.");
                    break;
            }
        }
    }
    
    
    //////////////////////////////////////////////////////////////////////////
	// 3. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
    Buffer = ConnectToClient(&h2, client);
	
	if(Buffer == NULL)
	{
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:UpdateBus:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}


	//////////////////////////////////////////////////////////////////////////
	// 4.1 Prepare the buffer for use to request list of buses
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_ListBuses) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:UpdateBus:ClientBlocked", "Server is still blocking the client channel! \n");
	}
	
    
	//////////////////////////////////////////////////////////////////////////
	// 4.2 Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, 0) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:UpdateBus:ServerTimedOut", "SERVER TIMED OUT! \n");
    }


	//////////////////////////////////////////////////////////////////////////
	// 4.3 Match Matlab data to MAPPS bus
	//////////////////////////////////////////////////////////////////////////
    
    if(strncmp(client->Result, "success", 7) == 0) {
        listResp = (DS_ListBusResponse *)Buffer;
        busID = -1;
        for (i = 0; i < (mwIndex)listResp->BusCount; i++) {
            if (strlen((const char *)Bus_string) == strlen((const char *)listResp->Buses[i].BusName) && 
                strcmp((const char *)Bus_string, (const char *)listResp->Buses[i].BusName) == 0) {
                busID = (int)i;
                break;
            }        
        }
        if (busID < 0) {
            DisconnectFromClient(h2, client);
            DisconnectFromGlobal(h1, global);
            mexErrMsgIdAndTxt("MEX:UpdateBus:NoSuchBus", "Bus %s does not exist. \n", Bus_string);
        }
        nElements = (mwIndex)listResp->Buses[busID].ElementCount;
        MAPPSElement_Names_ptr = (char **)mxCalloc(nElements, sizeof(char *));
        MAPPSElement_Type = (int *)mxCalloc(nElements, sizeof(int));
        MAPPSElement_SizeInBytes = (int *)mxCalloc(nElements, sizeof(int));
        MAPPSOrderOfFields = (int *)mxCalloc(nElements, sizeof(int));
        for (i = 0; i < nElements; i++) {
            Name_length = strlen(listResp->Buses[busID].Elements[i].ElementName) + 1;
            MAPPSElement_Names_ptr[i] = (char *)mxCalloc((mwSize)Name_length, sizeof(char));
            strcpy(MAPPSElement_Names_ptr[i], listResp->Buses[busID].Elements[i].ElementName);
        
            MAPPSElement_Type[i] = listResp->Buses[busID].Elements[i].Type;
            MAPPSElement_SizeInBytes[i] = listResp->Buses[busID].Elements[i].SizeInBytes;
            MAPPSOrderOfFields[i] = -1;
            for (j = 0; j < (mwIndex)nFields; j++) {
                if (strlen((const char *)FieldNames_ptr[j]) == strlen((const char *)MAPPSElement_Names_ptr[i]) &&
                    strcmp((const char *)FieldNames_ptr[j], (const char *)MAPPSElement_Names_ptr[i]) == 0) {
                    MAPPSOrderOfFields[i] = j;  // Matlab struct ensures that fields can't share the same name
                    break;
                }
            }
        }
        for (j = 0; j < (mwIndex)nFields; j++) {
            ignoreField = 1;
            for (i = 0; i < nElements; i++) {    
                if (MAPPSOrderOfFields[i] == j) {
                    ignoreField = 0;
                    break;
                }
            }
            if (ignoreField == 1)
                mexPrintf("Field %s does not exist for specified bus - ignoring field. \n", FieldNames_ptr[j]);
        }
    }
    else {
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:UpdateBus:ListBusesFail", "Message received = %s \n", client->Details);
	}   
    


	//////////////////////////////////////////////////////////////////////////
	// 5.1 Prepare the buffer for use to update bus contents
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_UpdateBus) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:UpdateBus:ClientBlocked", "Server is still blocking the client channel! \n");
	}
	
       
	//////////////////////////////////////////////////////////////////////////
	// 5.2 Populate request to update bus
	//////////////////////////////////////////////////////////////////////////
	
	req = (DS_UpdateBusRequest *)Buffer;
    bufferSize = 0;
    
    sprintf(req->BusName, Bus_string);
    sprintf(req->Mode, "update_both");
	req->Count = (int)nTimestamps;
    
    req->ElementCount = (int)nElements;

    for(i=0; i<nElements; i++)
	{
        sprintf(req->Elements[i].ElementName, MAPPSElement_Names_ptr[i]);
        req->Elements[i].Type = MAPPSElement_Type[i];
        req->Elements[i].SizeInBytes = MAPPSElement_SizeInBytes[i];
    }

	// ALTERNATIVE:
	// If you don't wish to populate the buffer, as above, you can set the
	// count to zero (see below). This will default back to the old behavior 
	// where the order was implied based on the bus order. You are required to
	// either set the field to zero OR explicitly populate the elements.

	//req->ElementCount = 0;
    
    bufferSize += sizeof(DS_UpdateBusRequest);
    
    Populate_Shared_Memory_Buffer((const void *)&req->Payload, &payloadSize, (const char *)Bus_string,
                                  (const uint64)nTimestamps, (const void *)Timestamps, (const mxClassID)Timestamp_Type,    
                                  (const int)nElements, (const char **)MAPPSElement_Names_ptr, (const int *)MAPPSElement_Type, (const int *)MAPPSElement_SizeInBytes,
                                  (const int *)MAPPSOrderOfFields, (const int)nFields,
                                  (const char **)FieldNames_ptr, (const void **)FieldData_ptr, (const mxClassID *)Field_Type, (const int *)Field_SizeInBytes,
                                  (void *)global, (void *)client, (void *)h1, (void *)h2);
    
    bufferSize += payloadSize;
 

	//////////////////////////////////////////////////////////////////////////
	// 5.2 Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:UpdateBus:ServerTimedOut", "SERVER TIMED OUT! \n");
    }
    
    
    //////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////
    
    if(strncmp(client->Result, "success", 7) != 0)
	    mexPrintf("Details (mxUpdateBus): %s\n", client->Details);

    
    //////////////////////////////////////////////////////////////////////////
	// 7. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////
    
    if (nlhs == 1) {
        dims[0] = 0;
        dims[1] = 1;
        plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
    }
    
    
	//////////////////////////////////////////////////////////////////////////
	// 8. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 9. Clean up allocated memory
	//////////////////////////////////////////////////////////////////////////

    for (i = 0; i < (mwIndex)nFields; i++)
        mxFree(FieldNames_ptr[i]);
    mxFree(FieldNames_ptr);
    mxFree(FieldData_ptr);
    mxFree(Field_Type);
    mxFree(Field_SizeInBytes);
    
    for (i = 0; i < nElements; i++)
        mxFree(MAPPSElement_Names_ptr[i]);
    mxFree(MAPPSElement_Names_ptr);
    mxFree(MAPPSElement_Type);
    mxFree(MAPPSElement_SizeInBytes);
    mxFree(MAPPSOrderOfFields);
}
