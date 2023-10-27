/***************************************************************
 *
 * Copyright (C) 2022, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/

#include "condor_common.h"
#include "condor_config.h"

#include <stdio.h>
#include <string>

#include <errno.h>
#include <string.h>

#include <filesystem>

#include "stl_string_utils.h"

#include "manifest.h"
#include "checksum.h"
#include "shortfile.h"

int
usage( const char * argv0 ) {
    fprintf( stderr, "%s: [function] [argument]\n", argv0 );
    fprintf( stderr, "where [function] is one of:\n" );
    fprintf( stderr, "  getNumberFromFileName\n" );
    fprintf( stderr, "  validateManifestFile\n" );
    fprintf( stderr, "  validateFilesListedIn\n" );
    fprintf( stderr, "  FileFromLine\n" );
    fprintf( stderr, "  ChecksumFromLine\n" );
    fprintf( stderr, "  compute_file_sha256_checksum\n" );
    return 1;
}

int
main( int argc, char ** argv ) {
    if( argc < 3 ) {
        return usage( argv[0] );
    }

    set_priv_initialize();
    config();
    // dprintf_set_tool_debug( "TOOL", 0 );

    std::string function = argv[1];
    std::string argument = argv[2];

    if( function == "deleteFilesStoredAt" ) {
        if( argc != 6 ) {
            return usage( argv[0] );
        }
    } else if( argc != 3 ) {
        return usage( argv[0] );
    }

    if( function == "getNumberFromFileName" ) {
        int no = manifest::getNumberFromFileName( argument );
        fprintf( stdout, "%d\n", no );
        return no;
    } else if( function == "validateManifestFile" ) {
        bool valid = manifest::validateManifestFile( argument );
        fprintf( stdout, "%s\n", valid ? "true" : "false" );
        return valid ? 0 : 1;
    } else if( function == "validateFilesListedIn" ) {
        std::string error;
        bool valid = manifest::validateFilesListedIn( argument, error );
        fprintf( stdout, "%s\n", valid ? "true" : "false" );
        if(! valid) {
            fprintf( stdout, "%s\n", error.c_str() );
        }
        return valid ? 0 : 1;
    } else if( function == "FileFromLine" ) {
        std::string file = manifest::FileFromLine( argument );
        fprintf( stdout, "%s\n", file.c_str() );
        return 0;
    } else if( function == "ChecksumFromLine" ) {
        std::string checksum = manifest::ChecksumFromLine( argument );
        fprintf( stdout, "%s\n", checksum.c_str() ) ;
        return 0;
    } else if( function == "compute_file_sha256_checksum" ) {
        std::string checksum;
        bool ok = compute_file_sha256_checksum( argument, checksum );
        if( ok ) {
            fprintf( stdout, "%s *%s\n", checksum.c_str(), argument.c_str() );
        }
        return ok ? 0 : 1;
    } else if( function == "createManifestFor" ) {
        std::string error;
        bool created = manifest::createManifestFor( argument, "MANIFEST", error );
        if(! created) {
            fprintf( stdout, "%s\n", error.c_str() );
        }
        return created ? 0 : 1;
    } else if( function == "deleteFilesStoredAt" ) {
        // `deleteFilesStoredAt location-stem file-stem
        //                      lowCheckpointNo highCheckpointNo`

        std::string locationStem = argv[2];
        std::string fileStem = argv[3];

        char * endptr = NULL;
        long lowCheckpointNo = strtol( argv[4], & endptr, 10 );
        if( endptr == argv[4] || *endptr != '\0' ) {
            fprintf( stderr, "'%s' is not a number, aborting.\n", argv[4] );
            return 1;
        }
        long highCheckpointNo = strtol( argv[5], & endptr, 10 );
        if( endptr == argv[5] || *endptr != '\0' ) {
            fprintf( stderr, "'%s' is not a number, aborting.\n", argv[5] );
            return 1;
        }

        std::filesystem::path fileStemPath(fileStem);
        std::filesystem::path parentPath = fileStemPath.parent_path();
        std::filesystem::path jobAdPath = parentPath / ".job.ad";


        std::string file;
        std::string error;
        bool deleted = true;
        std::string destination;
        for( long i = lowCheckpointNo; i <= highCheckpointNo; ++i ) {
            formatstr( destination, "%s/%.4ld", locationStem.c_str(), i );
            formatstr( file, "%s.%.4ld", fileStem.c_str(), i );

            // fprintf( stderr, "%s %s\n", destination.c_str(), file.c_str() );
            if(! manifest::deleteFilesStoredAt( destination, file, jobAdPath, error )) {
                fprintf( stdout, "%s\n", error.c_str() );
                deleted = false;
            }
        }

        if( deleted ) {
            fprintf( stderr, "Removing %s after successful clean-up.\n", jobAdPath.string().c_str() );
            std::filesystem::remove( jobAdPath );

            fprintf( stderr, "Removing %s after successful clean-up.\n", parentPath.string().c_str() );
            std::filesystem::remove( parentPath );
        }

        return deleted ? 0 : 1;
    } else {
        return usage( argv[0] );
    }
}
