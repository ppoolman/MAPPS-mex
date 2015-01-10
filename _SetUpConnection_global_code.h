/*=================================================================
 * _SetUpConnection_global_code.h 
 *
 *=================================================================*/


//////////////////////////////////////////////////////////////////////////
// Connect to the global state
//////////////////////////////////////////////////////////////////////////

global = ConnectToGlobal(&h1);

if(global == NULL)
    mexErrMsgTxt("Unable to connect to shared memory buffer (global state)! \n");

if(global->HeartBeat == 0)
{
    DisconnectFromGlobal(h1, global);
    mexErrMsgTxt("Server does not appear to be running. Start server before running this function! \n"); 
}