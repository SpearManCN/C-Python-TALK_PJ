#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <time.h>
// #include 


#define PORT 1056
#define BUFFERSIZE 1024
#define LOG_FILE "logfile.txt"
#define MAX_CLIENTS 30
typedef void (*func_ptr_t)( int, char[] );

// DB -----------------------------------------------------------------------------------------
MYSQL *conn;
// const char *server = 
// const char *user = 
// const char *password = 
// const char *database = 
char logData[ BUFFERSIZE ]; 
// --------------------------------------------------------------------------------------------

// Declaration --------------------------------------------------------------------------------
void * producer_function ( void * );
void * worker_function( void * );
void readLogic ( int, char[] );
void sendGroupInfo( char[], int, int );
void signUp ( char[], char[], int);
void login ( char[], char[], int );
void enterGroup ( char[], char[], int );
void sendEcho ( char[] );
void insertChatLog( char[], char[], char[], char[] );
FILE *log_file = NULL;
char logData[ BUFFERSIZE ];
int max_attempts = 10;
int attempt = 0;
time_t now;
// -------------------------------------------------------------------------------------------
// pointer func-------------------------------------------------------------------------------
void signUpStart( int, char[] );
void loginStart( int, char[] );
void sendGroupInfoStart( int, char[] );
void createGroupStart( int, char[] );
void removeGroupStart( int, char[] );
void joinGroupStart( int, char[] );
void withdrawGroupStart( int, char[] );
void enterGroupStart( int, char[] );
void leaveGroupStart( int, char[] );
void chatStart( int, char[] );
func_ptr_t funcArr[10] = { chatStart, signUpStart, loginStart, sendGroupInfoStart, createGroupStart, removeGroupStart
    , joinGroupStart, withdrawGroupStart, enterGroupStart, leaveGroupStart};
// -------------------------------------------------------------------------------------------
int main ( void )
{
    // Log init  --------------------------------------------------------------------------------
    sa_initlog ( "SERVER01" , "Server01" ) ;
    sa_log( 1 , "PROGRAM START.....\n") ;
    // ------------------------------------------------------------------------------------------
    // socket cfg -------------------------------------------------------------------------------
    struct sockaddr_in server_t;
    int nErr = 0;
    int fd_size = 5;
    int server_socket = 0;
    server_t.sin_family = AF_INET;
    server_t.sin_port = htons( PORT );
    server_t.sin_addr.s_addr = htonl( INADDR_ANY);
    // ------------------------------------------------------------------------------------------

    char echo[ BUFFERSIZE ]; // server's echo buffer
    // DB init ----------------------------------------------------------------------------------
    conn = mysql_init(NULL);
    while (attempt < max_attempts) {
        if ( !mysql_real_connect ( conn, server, user, password, database, 3306, NULL, 0 ) ) {
            sa_log(1, "Db connect failed\n");
            attempt++;
            sleep(1);
        }else{
            sa_log(1, "Db connected successfully\n");
            attempt=0;
            break;
        }
    }
    // ------------------------------------------------------------------------------------------
    // socket init ------------------------------------------------------------------------------
    while (1) {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);

        if ( server_socket < 0 ) {
            sa_log(1,"Creating socket failed\n");
            sleep(5);
            continue;
        }
        sa_log(1,"Creating socket succeed\n");
        if ( bind( server_socket, (struct sockaddr *) &server_t, sizeof(server_t) ) < 0 ) {
            sa_log(1,"Binding failed\n");
            close(server_socket);
            sleep(5);
            continue;
        }
        sa_log(1, "Binding succeed\n");
        nErr = listen( server_socket, fd_size );
        if ( nErr < 0 ) {
            sa_log(1, "Listening failed\n");
            close(server_socket);
            sleep(5);
            continue;
        }
        sa_log (1,  "Listening started\n" );
        break;
    }
    // ------------------------------------------------------------------------------------------
    // thread init ------------------------------------------------------------------------------
    pthread_t worker_thread;
    
    pthread_create ( &worker_thread, NULL, worker_function, &server_socket );
    // ------------------------------------------------------------------------------------------
    // server's echo 
    memset(echo, 0, BUFFERSIZE);
    while(1){
        fgets(echo, BUFFERSIZE, stdin);
        sendEcho(echo);
    }
    // ------------------------------------------------------------------------------------------
    mysql_close( conn ) ;
    return 0;
}

// worker thread---------------------------------------------------------------------------------
void * worker_function ( void * arg ) {
    int server_socket = * ( ( int * ) arg );
    char readBuffer[ BUFFERSIZE ];
    // file descriptor setting-------------------------------------------------------------------
    fd_set readfds;
    int max_sd, sd, activity, new_socket, valread, i, j;
    int sockets[MAX_CLIENTS];
    // ------------------------------------------------------------------------------------------
    // init socket array-------------------------------------------------------------------------
    for (i = 0; i < MAX_CLIENTS; i++) {
        sockets[i] = -1;
    }
    sockets[0] = server_socket;
    // ------------------------------------------------------------------------------------------
    // select function start---------------------------------------------------------------------
    while ( 1 ) {
        FD_ZERO(&readfds);
        max_sd = 0;
        for (i = 0; i < MAX_CLIENTS; i++) { // FD init
            sd = sockets[i];
            if (sd >= 0) {
                FD_SET(sd, &readfds); 
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            sa_log(1, "select error\n");
            continue;
        }
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = sockets[i];
            if (FD_ISSET(sd, &readfds)) { // request detection
                if( sd == server_socket ){
                    struct sockaddr_in cli_t;
                    int cli_size = sizeof( cli_t );
                    int success_socket = accept( server_socket, (struct sockaddr *) &cli_t, (socklen_t *)&cli_size);
                    for (j = 0; j < MAX_CLIENTS; j++) {
                        if (sockets[j] < 0) {
                            sockets[j] = success_socket;
                            break;
                        }
                    }
                }else{
                    sa_log(1, " selected read socket : %d\n", sd);
                    valread = read(sd, readBuffer, sizeof(readBuffer)); // valread = request occurred
                    if (valread <= 0) {                                  // socket disconnected
                        close(sd);
                        sockets[i] = -1; 
                    } else {
                        readLogic( sd, readBuffer);                     // response logic
                    }
                }
            }
        }
    }
}

void readLogic ( int success_socket, char readBuffer[] ) {
    char readHeader = readBuffer[0];                        // request type header
    sa_log(1, "readLogic started buffer = %s\n", readBuffer);
    funcArr[readHeader-'0']( success_socket, readBuffer);   // start response logic
}

// funcArr --------------------------------------------------------------------------------//

void signUpStart( int success_socket, char readBuffer[] ){
    char id[20];
    char pw[20];
    int tmp = readBuffer[1]-'0';
    strcpy(id, readBuffer+3); 
    strcpy(pw, readBuffer+4+tmp);
    signUp(id, pw, success_socket); // Check for duplicates, insert newId into DB
}

void loginStart( int success_socket, char readBuffer[] ){
    char id[20];
    char pw[20];
    int tmp = readBuffer[1]-'0';
    strcpy(id, readBuffer+3);
    strcpy(pw, readBuffer+4+tmp);
    login(id, pw, success_socket); // Validation
}

void sendGroupInfoStart( int success_socket, char readBuffer[] ){
    char id[20];
    strcpy(id, readBuffer+2);
    if( readBuffer[1]-'0' == 1){    // joinedGroupList
        sendGroupInfo(id, 1, success_socket);
    }else{                          // notJoinedGroupList
        sendGroupInfo(id, 2, success_socket);
    }
}

void createGroupStart( int success_socket, char readBuffer[] ){
    char id[20];
    char groupName[50];
    int tmp = readBuffer[1]-'0';
    strcpy(id, readBuffer+2);
    strcpy(groupName, readBuffer+3+tmp);
    createGroup(id, groupName, success_socket); // Check for duplicates, Create group
}

void removeGroupStart( int success_socket, char readBuffer[] ){
    char id[20];
    char groupName[50];
    if( readBuffer[1]-'0' == 1 ){           // send joinedGroupList (for remove)
        strcpy(id, readBuffer+2);
        sendGroupInfo(id, 3, success_socket);
    }else if( readBuffer[1]-'0' == 2 ){     // send valCanRemove (validation)
        int tmp = readBuffer[2]-'0';
        strcpy(id, readBuffer+3);
        strcpy(groupName, readBuffer+4+tmp);
        valCanRemove(id, groupName, success_socket);
    }else if( readBuffer[1]-'0' == 3 ){     // void. just removeGroup
        int tmp = readBuffer[2]-'0';
        strcpy(id, readBuffer+3);
        strcpy(groupName, readBuffer+4+tmp);
        removeGroup(id, groupName, success_socket);
    }
}

void joinGroupStart( int success_socket, char readBuffer[] ){
    char id[20];
    char groupName[50];
    int tmp = readBuffer[1]-'0';
    strcpy(id, readBuffer+2);
    strcpy(groupName, readBuffer+3+tmp);
    joinGroup(id, groupName, success_socket);   // validation, insert user into group (join)
}

void withdrawGroupStart( int success_socket, char readBuffer[] ){
    char id[20];
    char groupName[50];
    if( readBuffer[1]-'0' == 1){        //send joinedGroupList 
        strcpy(id, readBuffer+2);
        withdrawGroup(id, success_socket);
    }else if( readBuffer[1]-'0' == 2 ){ 
        // validation, send result.(valCanDelete)
        int tmp = readBuffer[2]-'0';
        strcpy(id, readBuffer+3);
        strcpy(groupName, readBuffer+4+tmp);
        valCanDelete(id, groupName, success_socket);
    }else { // void. just withdraw Group. (delete user from group)
        int tmp = readBuffer[2]-'0';
        strcpy(id, readBuffer+3);
        strcpy(groupName, readBuffer+4+tmp);
        deleteJoinedGroup(id, groupName, success_socket);
    }
}

void enterGroupStart( int success_socket, char readBuffer[] ){
    char id[20];
    char groupName[50];
    if( readBuffer[1]-'0' == 1){    
        // validation. (Is this user a member of the group)
        int tmp = readBuffer[2]-'0';
        strcpy(id, readBuffer+3);
        strcpy(groupName, readBuffer+4+tmp);
        enterGroup(id, groupName, success_socket);
    }else if(readBuffer[1]-'0' == 2){
        // send the group's memberList
        int tmp = readBuffer[2]-'0';
        strcpy(id, readBuffer+3);
        strcpy(groupName, readBuffer+4+tmp);
        sendMemberNow(id, groupName, success_socket);
    }else if( readBuffer[1]-'0' == 3){
        int tmp = readBuffer[2]-'0';
        strcpy(id, readBuffer+3);
        strcpy(groupName, readBuffer+4+tmp);
        sendPreChat(groupName, success_socket);
    }
}

void leaveGroupStart( int success_socket, char readBuffer[] ){
     // delete user from group's nowMember (leave)
    char id[20];
    char groupName[50];
    int tmp = readBuffer[1]-'0';
    strcpy(id, readBuffer+2);
    strcpy(groupName, readBuffer+3+tmp);
    leaveGroup(id, groupName, success_socket);
}

void chatStart( int success_socket, char readBuffer[] ){
    char id[20];
    char groupName[50];
    char beforeAtoi[10];
    char message[ BUFFERSIZE ];
    if( readBuffer[1]-'0' == 1 ){       
        // newUser has joined this chatting room
        int tmp = readBuffer[2]-'0';
        strcpy(id, readBuffer+3);
        strcpy(groupName, readBuffer+4+tmp);
        sendMessage(id, groupName, "/quit");
    }else if( readBuffer[1]-'0' == 2 ){ // a User has left
        int tmp1 = readBuffer[2]-'0';
        strcpy(beforeAtoi, readBuffer+4+tmp1);
        int tmp2 = atoi(beforeAtoi)-10;
        strcpy(id, readBuffer+3);
        strcpy(groupName, readBuffer+7+tmp1);
        strcpy(message, readBuffer+8+tmp1+tmp2);
        sendMessage(id, groupName, message);
    }else{                              
        // send chatting message to this group's nowMembers
        int tmp = readBuffer[2]-'0';
        strcpy(id, readBuffer+3);
        strcpy(groupName, readBuffer+4+tmp);
        sendMessage(id, groupName, "/quit/quit");
    }
}
// --------------------------------------------------------------------------------------------//

void sendPreChat ( char groupName[], int success_socket ){
    char query[ BUFFERSIZE ];
    char writeBuffer[ BUFFERSIZE ];
    sprintf( query, "select chat from chatlog where groupName = '%s'", groupName);
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        return -1;
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
        return -1;
    }
    MYSQL_ROW row; 
    while( (row = mysql_fetch_row(result)) ){
        sprintf ( writeBuffer, "%s", row[0] );
        // snprintf(writeBuffer, sizeof(writeBuffer), "%s", row[0]);
        write ( success_socket, writeBuffer, sizeof(writeBuffer) );
    }
}

char findUser ( char id[], char pw[] ) {
    char query[512];
    sprintf( query, "select count(*) from user where id = '%s' and pw = '%s' ", id, pw);
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        return -1;
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
        return -1;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL) {
        sa_log(1, "fetch row failed %s\n", __func__);
        return -1;
    }
    mysql_free_result(result);
    return *row[0];
}

void sendEcho(char echo[]){ // server echo logic
    char query[ BUFFERSIZE ];
    char writeBuffer[ BUFFERSIZE ];
    size_t len = strlen(echo);
    if (len > 0 && echo[len - 1] == '\n') {
        echo[len - 1] = '\0';
    }
    sprintf( query, "select memberSocket from groups where memberSocket is not null");
    sprintf( writeBuffer, "--Notice-- %s ",echo);
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        return -1;
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
        return -1;
    }
    MYSQL_ROW row; 
    char tempBuffer[16];
    while( (row = mysql_fetch_row(result)) ){
        memset(tempBuffer, 0, 16);
        snprintf(tempBuffer, sizeof(tempBuffer), "%s", row[0]);
        // send server's message (echo) to all group's nowMembers
        write ( atoi(tempBuffer), writeBuffer , sizeof(writeBuffer) ) ;
    }
}

char valCanJoin(char id[], char groupName[]){
    char query[512];
    sprintf( query, "SELECT case WHEN EXISTS (SELECT 1 FROM groups g where g.groupName = '%s') AND NOT EXISTS (SELECT 1 FROM groups g WHERE g.groupName = '%s' AND memberJoin = '%s') THEN 1 ELSE 0 END as a;", groupName, groupName, id );
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit(1);
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
        exit(1);
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL) {
        sa_log(1, "fetch row failed %s\n", __func__);
        exit(1);
    }
    if( * row[0] == '0' ){  // false (can't join)
        sa_log(1, "failed %s\n", __func__);
        return '0';
    }else{                  // true (can join)
        sa_log(1, "success %s\n", __func__);
        return '1'; 
    }
}

void valCanRemove( char id[], char groupName[], int success_socket ){
    char writeBuffer[BUFFERSIZE];
    char query[512];
    sprintf( query, "SELECT case WHEN not EXISTS (select 1 from groups g where g.groupName = '%s' and g.memberJoin is null and g.memberNow !='%s') AND EXISTS (SELECT 1 FROM groups g WHERE g.groupName = '%s' AND memberJoin = '%s') THEN 1 ELSE 0 END as a;", groupName, id , groupName, id );
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit(1);
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
        exit(1);
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL) {
        sa_log(1, "fetch row failed %s\n", __func__);
        exit(1);
    }
    if( * row[0] == '1' ){  // true (can remove)
        sa_log(1, "valCanRemove success\n");
        write ( success_socket, "1" , sizeof("1") ) ;
    }else{                  // false (can't remove)
        sa_log(1, "valCanRemove failed\n"); 
        write ( success_socket, "0" , sizeof("0") ) ;
    }
}

char valJoinedGroup(char id[], char groupName[]){
    char query[512];
    sprintf( query, "select count(*) from groups where groupName = '%s' and memberJoin = '%s' ", groupName, id );
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
        exit(1);
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL) {
        sa_log(1, "fetch row failed %s\n", __func__);
        exit(1);
    }
    if( * row[0] == '0' ){  // false (This user does not belong to this group)
        sa_log(1, "failed %s\n", __func__);
        return '0';
    }else{                  // true (This user is a member of the group)
        sa_log(1, "success %s\n", __func__);
        return '1';
    }
}

char valGroup(char groupName[]){
    char query[512];
    sprintf( query, "select count(*) from groups where groupName = '%s' ", groupName );
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
        exit(1);
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL) {
        sa_log(1, "fetch row failed %s\n", __func__);
        exit(1);
    }
    if( * row[0] == '0' ){  // true (can use this groupName for creating a new group)
        sa_log(1, "good %s\n", __func__);
        
        return '0';
    }else{                  // false (can't use this groupName)
        sa_log(1, "already exists %s\n", __func__);
        return '1';
    }
}

void valCanDelete( char id[], char groupName[], int success_socket ){
    if ( valJoinedGroup( id, groupName ) == '1' ){  // true (this user is a member of the group)
        write ( success_socket, "1" , sizeof("0") ) ;
    }else{                                          // false (not joined)
        write ( success_socket, "0" , sizeof("0") ) ;
    }
}

void insertUser ( char id[], char pw[] ){
    char query[512];
    sprintf( query, "INSERT INTO user (id, pw) VALUES ('%s', '%s')",
            id, pw);
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit( 1 );
    }
}

void insertMemberNow( char id[], char groupName[], int success_socket ){
    char query[512];
    sprintf( query, "INSERT INTO groups ( groupName, memberNow, memberSocket ) VALUES ('%s', '%s','%d')",
            groupName, id, success_socket);
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit( 1 );
    }
}

void signUp( char id[], char pw[], int success_socket ){
    char result = findUser(id,pw);
    char writeBuffer[BUFFERSIZE];
    
    if ( result == '1' ){   // false (This id already exists)
        write ( success_socket, "0" , sizeof("0") ) ;
        sa_log (1,  "This id already exists.\n" );
    }else {                 // true (create new id)
        insertUser( id, pw);
        write ( success_socket, "1" , sizeof("0") ) ;
        
    }
}
void login( char id[], char pw[], int success_socket ){
    char result = findUser(id,pw);
    if ( result == '1' ){   // Login success
        write ( success_socket, "1" , sizeof("0") ) ;
        sa_log (1,  "Login success\n" );
    }else {                 // wrong id
        write ( success_socket, "0" , sizeof("0") ) ;
        sa_log (1,  "wrong id\n");
    }
}

void sendMemberNow(char id[], char groupName[], int success_socket){
    insertMemberNow( id, groupName, success_socket );   // insert this user into the group 
    char query[512];
    char writeBuffer[BUFFERSIZE];
    sprintf( query, 
    "select memberNow from groups where groupName = '%s' and memberNow is not null ;", groupName );
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
        exit(1);
    }
    MYSQL_ROW row; 
    memset(writeBuffer, 0, BUFFERSIZE);
    char tempBuffer[100];
    // send userList of this group (nowMembers)
    while( (row = mysql_fetch_row(result)) ){
        snprintf(tempBuffer, sizeof(tempBuffer), "%s,", row[0]);
        strcat(writeBuffer, tempBuffer); 
    }
    write ( success_socket, writeBuffer , sizeof(writeBuffer) ) ;

}

void sendGroupInfo( char id[], int type, int success_socket ){
    // DB query ----------------------------------------------------------------------------------------------
    char query[512];
    char writeBuffer[BUFFERSIZE];
    if( type == 1 ){    // my joined, now mem
        sprintf( query, 
        "SELECT groupName, COUNT(memberNow) AS members FROM groups WHERE groupName IN (SELECT groupName FROM groups WHERE memberJoin = '%s') GROUP BY groupName;", id  );
    }else if( type == 2 ){              // not my joined, joined mem
        sprintf( query, 
        "SELECT groupName, COUNT(memberJoin) AS members FROM groups WHERE groupName NOT IN (SELECT groupName FROM groups WHERE memberJoin = '%s') GROUP BY groupName;", id  );
    }else if( type == 3 ){ // my joined, joined mem, now mem
        sprintf( query, 
        "SELECT groupName, COUNT(memberJoin), COUNT(memberNow) AS members FROM groups WHERE groupName IN (SELECT groupName FROM groups WHERE memberJoin = '%s') GROUP BY groupName;", id  );
    }
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
        exit(1);
    }
    MYSQL_ROW row; 
    //---------------------------------------------------------------------------------------------------------
    memset(writeBuffer, 0, BUFFERSIZE);
    char tempBuffer[100];
    if ( type == 3 ){   // send [groupName, number of joinedMembers, number of nowMembers]
        while( (row = mysql_fetch_row(result)) ){
            snprintf(tempBuffer, sizeof(tempBuffer), "%s:%s:%s,", row[0], row[1], row[2]);
            strcat(writeBuffer, tempBuffer);
        }
    }else{              // send [groupName, number of joinedMembers] or [groupName, number of nowMembers]
        while( (row = mysql_fetch_row(result)) ){
            snprintf(tempBuffer, sizeof(tempBuffer), "%s:%s,", row[0], row[1]);
            strcat(writeBuffer, tempBuffer);
        }
    }
    write ( success_socket, writeBuffer , sizeof(writeBuffer) ) ;
}



void createGroup( char id[], char groupName[], int success_socket ){
    char query[512];
    char valResult = valGroup( groupName );
    if ( valResult == '1'){ // failed (already exists)
        write ( success_socket, "0", sizeof("0") ) ;
    }else{                  // success (create new group)
        write ( success_socket, "1", sizeof("1") ) ;
        sprintf( query, "INSERT INTO groups (groupName, memberJoin) VALUES ('%s', '%s')",
            groupName, id );
        if ( mysql_query( conn, query )) {
            sa_log(1, "query failed %s\n", __func__);
            exit( 1 );
        }
    }
}

void removeGroup( char id[], char groupName[], int success_socket ){
    char query[512];
    sprintf( query, "delete from groups where groupName = '%s' ", groupName);
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit( 1 );
    }
}
void joinGroup( char id[], char groupName[], int success_socket ){
    char valResult = valCanJoin(id, groupName);
    char query[512];
    if(valResult=='1'){ // success (can join)
        sprintf( query, "INSERT INTO groups ( groupName, memberJoin) VALUES ('%s', '%s')", groupName, id);
        if ( mysql_query( conn, query )) {
            sa_log(1, "query failed %s\n", __func__);
            exit( 1 );
        }
        write ( success_socket, "1", sizeof("1") ) ;
    }else{              // failed (can't join)
        write ( success_socket, "0", sizeof("0") ) ;
    }
}
void withdrawGroup(char id[], int success_socket){
    // DB query -----------------------------------------------------------------------------------------
    char query[512];
    char writeBuffer[BUFFERSIZE];
    sprintf( query, 
        "SELECT groupName, COUNT(memberJoin) AS members FROM groups WHERE groupName IN (SELECT groupName FROM groups WHERE memberJoin = '%s') GROUP BY groupName;", id  );
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
        exit(1);
    }
    MYSQL_ROW row; 
    // --------------------------------------------------------------------------------------------------
    memset(writeBuffer, 0, BUFFERSIZE);
    char tempBuffer[100];
    while( (row = mysql_fetch_row(result)) ){   // send [groupName, number of joinedMembers]
        snprintf(tempBuffer, sizeof(tempBuffer), "%s:%s,", row[0], row[1]);
        strcat(writeBuffer, tempBuffer);
    }
    write ( success_socket, writeBuffer , sizeof(writeBuffer) ) ;
}

void deleteJoinedGroup( char id[], char groupName[], int success_socket ){
    char query[512];
    char writeBuffer[BUFFERSIZE];
    sprintf( query, 
        "delete from groups where memberJoin = '%s' and groupName = '%s' ", id, groupName  );
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit( 1 );
    }
}


void enterGroup( char id[], char groupName[], int success_socket){
    char valResult = valJoinedGroup(id, groupName);
    if(valResult=='1'){ // success
        write ( success_socket, "1", sizeof("1") ) ;
    }else{ // failed
        write ( success_socket, "0", sizeof("0") ) ;
    }
}

void sendMessage( char id[], char groupName[], char message[]){
    char query[512];
    char writeBuffer[BUFFERSIZE];
    char newMem[ BUFFERSIZE ];
    int isNew = 0;
    // time var init-------------------------------------------------------------------------------
    char timeBuffer[ BUFFERSIZE ];
    char timeBufferYear[ BUFFERSIZE ];
    char timeForDb[ BUFFERSIZE ];
    time(&now);
    struct tm * local = localtime(&now);
    strftime(timeBuffer, BUFFERSIZE, "%H:%M ", local);
    strftime(timeBufferYear, BUFFERSIZE, "%Y-%m-%d %H:%M ", local);
    strftime(timeForDb, BUFFERSIZE, "%Y%m%d%H%M%S", local);
    strcat(timeBufferYear, id);
    //---------------------------------------------------------------------------------------------
    // message processing(join,leave)--------------------------------------------------------------
    if( message == "/quit" ){               // new User has joined
        sprintf( newMem, "/quit%s", timeBufferYear);
        isNew = 1;
    }else if( message == "/quit/quit" ){    // user has left
        sprintf( newMem, "/quit/quit%s", timeBufferYear);
        isNew = 2;
    }
    //---------------------------------------------------------------------------------------------
    //query----------------------------------------------------------------------------------------
    sprintf( query, 
        "select distinct memberSocket from groups where groupName = '%s' and memberNow is not null;", groupName  );
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        sa_log(1, "store result failed %s\n", __func__);
    }
    MYSQL_ROW row; 
    //---------------------------------------------------------------------------------------------
    memset(writeBuffer, 0, BUFFERSIZE);
    char tempBuffer[ BUFFERSIZE ];
    snprintf(writeBuffer, sizeof(tempBuffer), "%s : ", id);
    strcat(timeBuffer, writeBuffer);
    strcat(timeBuffer, message);

    while( (row = mysql_fetch_row(result)) ){
        memset(tempBuffer, 0, BUFFERSIZE);
        snprintf(tempBuffer, sizeof(tempBuffer), "%s", row[0]);
        if ( isNew == 1 ){      // new user
            sa_log(1, " join message sended\n");
            write ( atoi(tempBuffer), newMem , sizeof(newMem) ) ;
        }else if( isNew == 2){  // user left
            sa_log(1, " left message sended\n");
            write ( atoi(tempBuffer), newMem , sizeof(newMem) ) ;
        }else{                  // user chat
            sa_log(1, " nomal message sended\n");
            write ( atoi(tempBuffer), timeBuffer , sizeof(timeBuffer) ) ;
        }
    }
    if( isNew == 1 ){       //new user
        insertChatLog( id, newMem, timeForDb, groupName );
    }else if( isNew == 2 ){ //user left
        insertChatLog( id, newMem, timeForDb, groupName );
    }else{                  //user chat
        insertChatLog( id, timeBuffer, timeForDb, groupName );
    }
}

void insertChatLog( char id[], char chat[], char time[], char groupName[] ){
    char query[ BUFFERSIZE ];
    sprintf( query, 
        "insert into chatlog ( user, chat, time, groupName ) values ( '%s', '%s', '%s', '%s' )", id, chat, time, groupName  );
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
    }
}

void leaveGroup(char id[], char groupName[], int success_socket){
    char query[512];
    char writeBuffer[BUFFERSIZE];
    sprintf( query, 
        "delete from groups where memberNow = '%s' and groupName = '%s' ", id, groupName  );
    if ( mysql_query( conn, query )) {
        sa_log(1, "query failed %s\n", __func__);
        exit( 1 );
    }
}
