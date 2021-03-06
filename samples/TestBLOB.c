//-----------------------------------------------------------------------------
// Copyright (c) 2016, 2017 Oracle and/or its affiliates.  All rights reserved.
// This program is free software: you can modify it and/or redistribute it
// under the terms of:
//
// (i)  the Universal Permissive License v 1.0 or at your option, any
//      later version (http://oss.oracle.com/licenses/upl); and/or
//
// (ii) the Apache License v 2.0. (http://www.apache.org/licenses/LICENSE-2.0)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// TestBLOB.c
//   Tests whether BLOBs are handled properly using ODPI-C.
//-----------------------------------------------------------------------------

#include "Test.h"
#define SQL_TEXT_1                      "truncate table TestBLOBs"
#define SQL_TEXT_2                      "insert into TestBLOBs values (:1, :2)"
#define SQL_TEXT_3                      "select IntCol, BlobCol from TestBLOBs"
#define NUM_ROWS                        10
#define LOB_SIZE_INCREMENT              25000
#define MAX_LOB_SIZE                    NUM_ROWS * LOB_SIZE_INCREMENT

//-----------------------------------------------------------------------------
// main()
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
    uint32_t numQueryColumns, bufferRowIndex, i;
    dpiData *intColValue, *blobColValue;
    dpiVar *intColVar, *blobColVar;
    dpiNativeTypeNum nativeTypeNum;
    char buffer[MAX_LOB_SIZE];
    dpiQueryInfo queryInfo;
    uint64_t blobSize;
    dpiStmt *stmt;
    dpiConn *conn;
    int found;

    // connect to database
    conn = GetConnection(0, NULL);
    if (!conn)
        return -1;

    // truncate table
    if (dpiConn_prepareStmt(conn, 0, SQL_TEXT_1, strlen(SQL_TEXT_1), NULL, 0,
            &stmt) < 0)
        return ShowError();
    if (dpiStmt_execute(stmt, 0, &numQueryColumns) < 0)
        return ShowError();
    if (dpiStmt_release(stmt) < 0)
        return ShowError();

    // populate with a number of rows
    if (dpiConn_prepareStmt(conn, 0, SQL_TEXT_2, strlen(SQL_TEXT_2), NULL, 0,
            &stmt) < 0)
        return ShowError();
    if (dpiConn_newVar(conn, DPI_ORACLE_TYPE_NUMBER, DPI_NATIVE_TYPE_INT64,
            1, 0, 0, 0, NULL, &intColVar, &intColValue) < 0)
        return ShowError();
    if (dpiConn_newVar(conn, DPI_ORACLE_TYPE_LONG_RAW, DPI_NATIVE_TYPE_BYTES,
            1, 0, 0, 0, NULL, &blobColVar, &blobColValue) < 0)
        return ShowError();
    if (dpiStmt_bindByPos(stmt, 1, intColVar) < 0)
        return ShowError();
    if (dpiStmt_bindByPos(stmt, 2, blobColVar) < 0)
        return ShowError();
    intColValue->isNull = 0;
    blobColValue->isNull = 0;
    for (i = 0; i < NUM_ROWS; i++) {
        intColValue->value.asInt64 = i + 1;
        memset(buffer, i + 'A', LOB_SIZE_INCREMENT * (i + 1));
        if (dpiVar_setFromBytes(blobColVar, 0, buffer,
                LOB_SIZE_INCREMENT * (i + 1)) < 0)
            return ShowError();
        if (dpiStmt_execute(stmt, 0, &numQueryColumns) < 0)
            return ShowError();
    }
    if (dpiStmt_release(stmt) < 0)
        return ShowError();
    if (dpiVar_release(intColVar) < 0)
        return ShowError();
    if (dpiVar_release(blobColVar) < 0)
        return ShowError();

    // fetch rows
    if (dpiConn_prepareStmt(conn, 0, SQL_TEXT_3, strlen(SQL_TEXT_3), NULL, 0,
            &stmt) < 0)
        return ShowError();
    if (dpiStmt_execute(stmt, 0, &numQueryColumns) < 0)
        return ShowError();
    while (1) {
        if (dpiStmt_fetch(stmt, &found, &bufferRowIndex) < 0)
            return ShowError();
        if (!found)
            break;
        if (dpiStmt_getQueryValue(stmt, 1, &nativeTypeNum, &intColValue) < 0 ||
                dpiStmt_getQueryValue(stmt, 2, &nativeTypeNum,
                        &blobColValue) < 0)
            return ShowError();
        if (dpiLob_getSize(blobColValue->value.asLOB, &blobSize) < 0)
            return ShowError();
        printf("Row: IntCol = %g, BlobCol = BLOB(%" PRIu64 ")\n",
                intColValue->value.asDouble, blobSize);
    }

    // display description of each variable
    for (i = 0; i < numQueryColumns; i++) {
        if (dpiStmt_getQueryInfo(stmt, i + 1, &queryInfo) < 0)
            return ShowError();
        printf("('%*s', %d, %d, %d, %d, %d, %d)\n", queryInfo.nameLength,
                queryInfo.name, queryInfo.oracleTypeNum, queryInfo.sizeInChars,
                queryInfo.clientSizeInBytes, queryInfo.precision,
                queryInfo.scale, queryInfo.nullOk);
    }

    // clean up
    dpiStmt_release(stmt);
    dpiConn_release(conn);

    printf("Done.\n");
    return 0;
}

