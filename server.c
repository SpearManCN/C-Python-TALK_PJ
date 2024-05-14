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

#define port 1056
#define bufferSize 1024

struct user{
    int success_socket;
    char id[20];
};
struct group{
    int groupNo;
    struct user users[20];
};
struct room{
    int roomNo;
    struct user users[20];
};

MYSQL *conn;
// const char *server = "";
// const char *user = "";
// const char *password = "";
// const char *database = "";

void * logic ( void * );
void * readLogic ( void * );
void sendGroupInfo( char[], int, int );
void signUp ( char[], char[], int);
void login ( char[], char[], int );
void enterGroup ( char[], char[], int );
int main ( void )
{

    struct sockaddr_in server_t;
    int nErr = 0;
    int fd_size = 5;
    int server_socket = 0;
    conn = mysql_init(NULL);
    if ( !mysql_real_connect ( conn, server, user, password, database, 3306, NULL, 0 ) ) {
        
        printf("db connect failed\n");
        exit( 1 );
    }

    server_t.sin_family = AF_INET;
    server_t.sin_port = htons( port );
    server_t.sin_addr.s_addr = htonl( INADDR_ANY);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if ( server_socket < 0 ) {
        printf ( "Creating socket failed \n" );
        return -1;
    }
    printf ( "Creating socket succeed \n" );
    if ( bind( server_socket, (struct sockaddr *) &server_t, sizeof(server_t) ) < 0 ) {
        printf( "Binding failed\n" );
        return -1;
    }
    printf( "Binding succeed\n" );
    nErr = listen( server_socket, fd_size );
    if ( nErr < 0 ) {
        printf ("Listening failed\n" );
        return -1;
    }
    printf ( "Listening started\n" );

    pthread_t thread;
    
    pthread_create ( &thread, NULL, logic, &server_socket );
    // logic ( server_socket );
    while(1){
        sleep(1);
    }

    mysql_close( conn ) ;
    return 0;
}

void * logic ( void * arg ) {
    int server_socket = * ( ( int * ) arg );
    int i = 0;
    int success_socket =0;
    while ( 1 ) {

        struct sockaddr_in cli_t;
        int cli_size = sizeof( cli_t );

        success_socket = accept( server_socket, (struct sockaddr *) &cli_t, (socklen_t *)&cli_size);
        if ( success_socket < 0 ) {
            printf ( "Failed to accept\n" );
            continue;
        }
        printf ( "Success to accpet userNo : %d \n", success_socket );
        // write ( success_socket, "writeBuffer", sizeof("writeBuffer") );
        pthread_t thread;
        pthread_create ( &thread, NULL, readLogic, &success_socket );
        
        // pthread_create ( &thread, NULL, readLogic, &success_socket );
        // pthread_create ( &thread[i+3], NULL, writeLogic, &success_socket );
        
    }
}


void * readLogic ( void * arg ) {
    int success_socket = * ( ( int * ) arg );
    char readBuffer[ bufferSize ];
    char writeBuffer[ bufferSize ];
    int tmp = 0;
    char id[20];
    char pw[20];
    char groupName[50];
    int bytes_received = 0;

    while ( 1 ) {
        tmp= 0;
        memset( id, 0, 20 );
        memset( pw, 0, 20 );
        memset( groupName, 0, 50 );
        memset( readBuffer, 0, bufferSize );
        if (( bytes_received = read( success_socket, readBuffer, bufferSize  )) <= 0 ) {
            printf ( "read failed\n" );
            close ( success_socket );
            return;
        }
        printf ( "read success\n" );

        char readHeader = readBuffer[0];
        switch( readHeader-'0' ){
            case 1: // signup
                tmp = readBuffer[1]-'0';
                strcpy(id, readBuffer+3);
                strcpy(pw, readBuffer+4+tmp);
                signUp(id, pw, success_socket);
                break;
            case 2: // login
                tmp = readBuffer[1]-'0';
                strcpy(id, readBuffer+3);
                strcpy(pw, readBuffer+4+tmp);
                login(id, pw, success_socket);
                break;
            case 3: // sendGroupInfo   
                strcpy(id, readBuffer+2);
                if( readBuffer[1]-'0' == 1){
                    sendGroupInfo(id, 1, success_socket);
                }else{
                    sendGroupInfo(id, 2, success_socket);
                }

                break;
            case 4: // createGroup
                tmp = readBuffer[1]-'0';
                strcpy(id, readBuffer+2);
                strcpy(groupName, readBuffer+3+tmp);
                printf("id = %s, gN = %s", id, groupName);
                createGroup(id, groupName, success_socket);
                break;
            case 5: // removeGroup
                break;
            case 6: // joinGroup
                tmp = readBuffer[1]-'0';
                strcpy(id, readBuffer+2);
                strcpy(groupName, readBuffer+3+tmp);
                printf("id = %s, gN = %s", id, groupName);
                joinGroup(id, groupName, success_socket);
                break;
            case 7: // withdrawGroup
                if( readBuffer[1]-'0' == 1){
                    strcpy(id, readBuffer+2);
                    withdrawGroup(id, success_socket);
                }else{
                    
                }
                break;
            case 8: // enterGroup
                if( readBuffer[1]-'0' == 1){
                    tmp = readBuffer[2]-'0';
                    strcpy(id, readBuffer+3);
                    strcpy(groupName, readBuffer+4+tmp);
                    printf("id = %s, gN = %s", id, groupName);
                    enterGroup(id, groupName, success_socket);
                }else{
                    tmp = readBuffer[2]-'0';
                    strcpy(id, readBuffer+3);
                    strcpy(groupName, readBuffer+4+tmp);
                    printf("id = %s, gN = %s", id, groupName);
                    sendMemberNow(id, groupName, success_socket);
                    // enterGroup(id, groupName, success_socket);
                }
                
                break;
            case 9: // leaveGroup
            
                break;
            case 10: // chat
                break;

        }
    }
}

char findUser ( char id[], char pw[] ) {
    char query[512];
    sprintf( query, "select count(*) from user where id = '%s' and pw = '%s' ", id, pw);
    if ( mysql_query( conn, query )) {
        printf("query failed\n");
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("store result failed\n");
        exit(1);
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL) {
        printf("fetch row failed\n");
        exit(1);
    }

    printf("Number of rows where id = '%s' and pw = '%s': %c\n", id, pw, * row[0]);
    
    // printf ( " size : %d\n", sizeof(row[0]));
    mysql_free_result(result);
    // mysql_close( conn ) ;
    return *row[0];
}

char valCanJoin(char id[], char groupName[]){
    char query[512];
    sprintf( query, "SELECT case WHEN EXISTS (SELECT 1 FROM groups g where g.groupName = '%s') AND NOT EXISTS (SELECT 1 FROM groups g WHERE g.groupName = '%s' AND memberJoin = '%s') THEN 1 ELSE 0 END as a;", groupName, groupName, id );
    if ( mysql_query( conn, query )) {
        printf("query failed\n");
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("store result failed\n");
        exit(1);
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL) {
        printf("fetch row failed\n");
        exit(1);
    }
    if( * row[0] == '0' ){ //failed
        printf("failed\n");
        return '0';
    }else{
        printf("success\n");
        return '1';
    }
}

char valJoinedGroup(char id[], char groupName[]){
    char query[512];
    sprintf( query, "select count(*) from groups where groupName = '%s' and memberJoin = '%s' ", groupName, id );
    if ( mysql_query( conn, query )) {
        printf("query failed\n");
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("store result failed\n");
        exit(1);
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL) {
        printf("fetch row failed\n");
        exit(1);
    }
    if( * row[0] == '0' ){ //failed
        printf("failed\n");
        return '0';
    }else{
        printf("success\n");
        return '1';
    }
}

char valGroup(char groupName[]){
    char query[512];
    sprintf( query, "select count(*) from groups where groupName = '%s' ", groupName );
    // sprintf( query, "INSERT INTO log0429 (time, Qtype, log, side ) VALUES (%ld, %d, '%s', %d)",
    //         timeLong, Q_TYPE, log, 2 );
    if ( mysql_query( conn, query )) {
        printf("query failed\n");
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("store result failed\n");
        exit(1);
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL) {
        printf("fetch row failed\n");
        exit(1);
    }
    if( * row[0] == '0' ){
        printf("good\n");
        
        return '0';
    }else{
        printf("already exists\n");
        return '1';
    }
}

void insertUser ( char id[], char pw[] ){
    char query[512];
    sprintf( query, "INSERT INTO user (id, pw) VALUES ('%s', '%s')",
            id, pw);
    if ( mysql_query( conn, query )) {
        printf("query failed\n");
        exit( 1 );
    }
    printf("success db");
}

void insertMemberNow( char id[], char groupName[] ){
    char query[512];
    sprintf( query, "INSERT INTO groups ( groupName, memberNow) VALUES ('%s', '%s')",
            groupName, id);
    if ( mysql_query( conn, query )) {
        printf("query failed\n");
        exit( 1 );
    }
}

void signUp( char id[], char pw[], int success_socket ){
    char result = findUser(id,pw);
    char writeBuffer[bufferSize];
    
    if ( result == '1' ){
        write ( success_socket, "0" , sizeof("0") ) ;
        //go signUp
        printf ( "This id already exists.\n" );
    }else {
        insertUser( id, pw);
        write ( success_socket, "1" , sizeof("0") ) ;
        
    }
}
void login( char id[], char pw[], int success_socket ){
    char result = findUser(id,pw);
    if ( result == '1' ){
        write ( success_socket, "1" , sizeof("0") ) ;
        //go signUp
        printf ( "Login success\n" );
    }else {
        write ( success_socket, "0" , sizeof("0") ) ;
        printf ( "wrong id\n");
    }
}

void sendMemberNow(char id[], char groupName[], int success_socket){
    insertMemberNow( id, groupName );
    char query[512];
    char writeBuffer[bufferSize];
    sprintf( query, 
    "select memberNow from groups where groupName = '%s' and memberNow is not null ;", groupName );
    if ( mysql_query( conn, query )) {
        printf("query failed\n");
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("store result failed\n");
        exit(1);
    }
    MYSQL_ROW row; 
    memset(writeBuffer, 0, bufferSize);
    char tempBuffer[100];
    while( (row = mysql_fetch_row(result)) ){
        snprintf(tempBuffer, sizeof(tempBuffer), "%s,", row[0]);
        strcat(writeBuffer, tempBuffer);
    }
    printf("writebuffer : %s\n", writeBuffer);
    write ( success_socket, writeBuffer , sizeof(writeBuffer) ) ;

}

void sendGroupInfo( char id[], int type, int success_socket ){
    char query[512];
    char writeBuffer[bufferSize];
    if( type == 1 ){    // my joined, now mem
        sprintf( query, 
        "SELECT groupName, COUNT(memberNow) AS members FROM groups WHERE groupName IN (SELECT groupName FROM groups WHERE memberJoin = '%s') GROUP BY groupName;", id  );
    }else{              // not my joined, joined mem
        sprintf( query, 
        "SELECT groupName, COUNT(memberJoin) AS members FROM groups WHERE groupName NOT IN (SELECT groupName FROM groups WHERE memberJoin = '%s') GROUP BY groupName;", id  );
    }
    if ( mysql_query( conn, query )) {
        printf("query failed\n");
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("store result failed\n");
        exit(1);
    }
    MYSQL_ROW row; 
    memset(writeBuffer, 0, bufferSize);
    char tempBuffer[100];
    while( (row = mysql_fetch_row(result)) ){
        snprintf(tempBuffer, sizeof(tempBuffer), "%s:%s,", row[0], row[1]);
        strcat(writeBuffer, tempBuffer);
    }
    printf("writebuffer : %s\n", writeBuffer);
    write ( success_socket, writeBuffer , sizeof(writeBuffer) ) ;
}
void createGroup( char id[], char groupName[], int success_socket ){
    //이름이 중복되는지 검사
    char query[512];
    char valResult = valGroup( groupName );
    if ( valResult == '1'){ //failed already exists
        write ( success_socket, "0", sizeof("0") ) ;
    }else{ // success
        write ( success_socket, "1", sizeof("1") ) ;
        sprintf( query, "INSERT INTO groups (groupName, memberJoin) VALUES ('%s', '%s')",
            groupName, id );
        if ( mysql_query( conn, query )) {
            printf("query failed\n");
            exit( 1 );
        }
    }
}
void removeGroup(){

}
void joinGroup(char id[], char groupName[], int success_socket){
    char valResult = valCanJoin(id, groupName);
    char query[512];
    if(valResult=='1'){ // success
        sprintf( query, "INSERT INTO groups ( groupName, memberJoin) VALUES ('%s', '%s')", groupName, id);
        if ( mysql_query( conn, query )) {
            printf("query failed\n");
            exit( 1 );
        }
        write ( success_socket, "1", sizeof("1") ) ;
        // 현재 방 이름, 멤버들 목록 가져오기
    }else{ // failed
        write ( success_socket, "0", sizeof("0") ) ;
    }
}
void withdrawGroup(char id[], int success_socket){
    char query[512];
    char writeBuffer[bufferSize];
    sprintf( query, 
        "SELECT groupName, COUNT(memberJoin) AS members FROM groups WHERE groupName IN (SELECT groupName FROM groups WHERE memberJoin = '%s') GROUP BY groupName;", id  );
    if ( mysql_query( conn, query )) {
        printf("query failed\n");
        exit( 1 );
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("store result failed\n");
        exit(1);
    }
    MYSQL_ROW row; 
    memset(writeBuffer, 0, bufferSize);
    char tempBuffer[100];
    while( (row = mysql_fetch_row(result)) ){
        snprintf(tempBuffer, sizeof(tempBuffer), "%s:%s,", row[0], row[1]);
        strcat(writeBuffer, tempBuffer);
    }
    printf("writebuffer : %s\n", writeBuffer);
    write ( success_socket, writeBuffer , sizeof(writeBuffer) ) ;
}
void enterGroup( char id[], char groupName[], int success_socket){
    char valResult = valJoinedGroup(id, groupName);
    printf( "valResult : %c \n", valResult);
    if(valResult=='1'){ // success
        // insertMemberNow( id, groupName );
        write ( success_socket, "1", sizeof("1") ) ;
        // 현재 방 이름, 멤버들 목록 가져오기
    }else{ // failed
        write ( success_socket, "0", sizeof("0") ) ;
    }
    
}
void leaveGroup(){

}