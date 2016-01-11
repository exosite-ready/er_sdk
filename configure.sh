#!/bin/bash

#set -x

MY_DIR=$( pwd )
SDK_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
PARAM_NUM=$#
BUILD_DIR=$SDK_PATH

OBJECT_LIST_GEN=false
SOURCE_LIST_GEN=false
FLAG_LIST_GEN=false
ADD_COMMENTS=true
INCLUDE_LIST_GEN=false
LIB_MODE=false
CLEAN_UP=true

# This array contains the valid properties can be
# contained by the SOURCES files
declare -a PROPERTIES=("MODULE_NAME" "SOURCES_HOME")

#PROPERTIES=(MODULE_NAME $OPTARG)
#PROPERTIES=(SOURCES_HOME $OPTARG)

# This array contains the paths where the script
# will be looking for the SOURCES files
declare -a SOURCES_PATH=("$SDK_PATH")

# This array contains the definitions that have to
# be added to the output (with -D prefix) based on
# the input parameters.
declare -a EXTERNAL_DEFINES=("HAVE_CONFIG_H")

#----------------------------------------------------------
# Function for exit due to fatal program error
# Accepts 1 argument:
#       string containing descriptive error message
#----------------------------------------------------------
error_exit()
{
    echo "${SCRIPT_NAME}: ${1:-"Unknown Error"}" 1>&2
    exit 1
}

#---------------------------------------------------------
# This function creates local variable from a parameter.
# It converts "y,yes,true" values to "1" and "n,no,false"
# values to "0" case insensitively.
#
# $1 parameter
#---------------------------------------------------------
create_variable()
{
    new_var=$( echo $1 | sed -n -E "s/([^=]+)=(.+)/\1=\2/p" )
    
    if [ "$new_var" != "" ]; then
    
        local name=$( echo  $new_var | sed -n -E "s/([[:print:]]+)=[[:print:]]+/\1/p" )
        local value=$( echo $new_var | sed -n -E "s/[[:print:]]+=([[:print:]]+)/\1/p" )
        local lowerCase=`echo $value | tr '[:upper:]' '[:lower:]'`

        case "$lowerCase" in
            "y") new_var="$name=1";;
            "n") new_var="$name=0";;
            "yes") new_var="$name=1";;
            "no") new_var="$name=0";;
            "true") new_var="$name=1";;
            "false") new_var="$name=0";;
            *) ;;
        esac
        
        eval $new_var
    else
        new_var=$1
        eval "$new_var=1"
    fi

    EXTERNAL_DEFINES=("${EXTERNAL_DEFINES[@]}" $new_var)
}

#----------------------------------------------------------
# Process the script options
#----------------------------------------------------------
process_opts () {
  while getopts ":osfild:e:b:c:" optname; do
      case "$optname" in
        "o") OBJECT_LIST_GEN=true;;
        "s") SOURCE_LIST_GEN=true;;
        "f") FLAG_LIST_GEN=true;;
        "i") INCLUDE_LIST_GEN=true;;
        "l") LIB_MODE=true;;
        "d") create_variable $OPTARG;;
        "e") SOURCES_PATH=("${SOURCES_PATH[@]}" $OPTARG);;
        "b") BUILD_DIR=$( cd "$OPTARG" && pwd );;
        "c") CLEAN_UP=$OPTARG;;
        "?") error_exit "Unknown option $OPTARG";;
        ":") error_exit "No argument value for option $OPTARG";;
        *) error_exit "Unknown error while processing options";;  # Should not occur
      esac
    done
    return $OPTIND
}

#----------------------------------------------------------
# Process script arguments
# Overwrite the default values with the 
# command line parameters in case of name 
# matching.
#----------------------------------------------------------
process_args () {
    for p in "$@";do
        create_variable $p 
    done
}

#---------------------------------------------------------
# This function is able to create variables from C style
# #defines. It parses the given file (e.g. a header file)
# and create variables based on the found #defines. The
# names and the values of these variables are reflect the
# standard "#define <NAME> <VALUE>" format.
#---------------------------------------------------------
parse_config_file()
{
    eval $( grep -E "^[[:space:]]*#define[[:space:]]+[a-zA-Z0-9_]+[[:space:]]+[.]*[ a-zA-Z0-9_]+[.]*" "$1" | \
            sed  -n -E "s/[[:space:]]*#define[[:space:]]+([a-zA-Z0-9_]+)[[:space:]]+[(\"]?([^()\"]*)[)\"]?/\1=\2/p" )
}

#---------------------------------------------------------
# This function verify that all of the needed config
# variables exist. It this function fount an error, it
# might mean that the config_port.h file is corrupted.
#---------------------------------------------------------
config_verification()
{
    if [ -z "$CONFIG_NETWORK_STACK" ]; then
        error_exit "CONFIG_NETWORK_STACK is unknown"
    fi
}

#---------------------------------------------------------
# Print the path of an object
# Params:
#   $1: Source file path belongs to the required object
#   $2: Home folder of the SOURCES file
#---------------------------------------------------------
print_object()
{
    OBJ_PATH=$( echo $1 | sed -n -E 's/[ \t]*(.*)\.c[ \t]*/\1.o/p')
    echo "    $2$OBJ_PATH \\"
}

#---------------------------------------------------------
# Print the path of a source file
# Params:
#   $1: Source file path
#   $2: Home folder of the SOURCES file
#---------------------------------------------------------
print_source_file()
{
    SRC_PATH=$( echo $1 | sed -n -E 's/[ \t]*([a-zA-Z0-9_]*)[ \t]*/\1/p')
    echo "    $2$SRC_PATH \\"
}

#---------------------------------------------------------
# Print adding of a compiler flag 
# Params:
#   $1: Compiler flag
#   $2: Home folder of the SOURCES file
#---------------------------------------------------------
print_compiler_flag()
{
   CMP_FLAG=$( echo $1 | sed -n -E 's/[ \t]*([a-zA-Z0-9_]*)[ \t]*/\1/p')
   echo "    -D$CMP_FLAG \\"
}

#---------------------------------------------------------
# Print adding an include path
# Params:
#   $1: Relative path from the folder of SOURCES file
#   $2: Home folder of the SOURCES file
#---------------------------------------------------------
print_include_path()
{
   CMP_INC=$( echo $1 | sed -n -E 's/[ \t]*([a-zA-Z0-9_]*)[ \t]*/\1/p')
   echo "    -I$2$CMP_INC \\"
}

#---------------------------------------------------------
# Print additional labels
# Params:
# $1 : Config type    (e.g SRC, INC, ...)
# $2 : Config name    (e.g CONFIG_HAS_SDK_CORE)
# $3 : Module name
#---------------------------------------------------------
print_label()
{
    case "$1" in
    
        "SRC")
            local new_lib=$3_$(echo $2 | sed -n -E 's/(CONFIG_)?(HAS_)?//p' | sed -E 's/_//g' | tr '[:upper:]' '[:lower:]')
            if [ "$LIB_MODE" = "true" ]; then
                echo
                echo "$new_lib.a_SRC += \\"
            else
                if [ -z "$LIB_LIST_SRC" ]; then
                    echo
                    echo ER_SDK_SRC = \\
                fi
            fi
            LIB_LIST_SRC="$LIB_LIST_SRC $new_lib.a"
            ;;
            
        "OBJ")
            local new_lib=$3_$(echo $2 | sed -n -E 's/(CONFIG_)?(HAS_)?//p' | sed -E 's/_//g' | tr '[:upper:]' '[:lower:]')
            if [ "$LIB_MODE" = "true" ]; then
                echo
                echo "$new_lib.a_OBJS += \\"
            else
                if [ -z "$LIB_LIST_OBJ" ]; then
                    echo
                    echo ER_SDK_OBJS = \\
                fi
            fi
            LIB_LIST_OBJ="$LIB_LIST_OBJ $new_lib.a"
            ;;
            
        "DEF") 
            if [ -z "$DEF_LABEL_ADDED" ]; then
                echo
                echo ER_SDK_FLAG = \\
                DEF_LABEL_ADDED=true
            fi
            ;;
            
        "INC") 
            if [ -z "$INC_LABEL_ADDED" ]; then
                echo
                echo ER_SDK_INC = \\
                INC_LABEL_ADDED=true
            fi
            ;;
            
        *) error_exit "Unknown error while processing options";;  # Should not occur
    esac
}

#---------------------------------------------------------
# This function parses one line of the SOURCES file. If it
# contains a valid < PROPERTY = value > format property
# setting, it convert it to a variable 
#  $1: one line from the SOURCES file
#  return: Found if valid property is found
#---------------------------------------------------------
parse_property()
{
    local hit=0
    local new_property=""
    
    #Only look for PROPERTY if there is a '<' at the beginning of the line
    local hit=$(echo $1 | sed -n -E "s/^<.*/1/p")
    
    if [[ $hit == 1 ]]; then
        # Try to find < PROPERTY = <name> > format tag
        for property in ${PROPERTIES[*]}; do
            new_property=$( echo $1 | sed -n -E "s/<($property=[[:print:]]+)>/\1/p")
            if [ "$new_property" != "" ]; then
                break
            fi
        done
        
        if [ "$new_property" != "" ]; then
            PROPERTY_NAME=$( echo $new_property | sed -n -E 's/([a-zA-Z0-9_]+)=[[:print:]]+/\1/p')
            PROPERTY_VALUE=$( echo $new_property | sed -n -E 's/[a-zA-Z0-9_]+=([[:print:]]+)/\1/p')
        else
            error_exit "Unknown property $1"
        fi
    fi

    return $hit
}
#---------------------------------------------------------
# This function parses one line of the SOURCES file. If it
# contains a valid [ TAG; config [=value] ] format tag
# it calls the label printer function.
#  $1: one line from the SOURCES file
#  $2: Name of the tag type (SRC, DEF, INC, etc...)
#  $3: Name of the printer function that will be called
#      if this function finds some data to print.
#  $4: Name of the group label type
#---------------------------------------------------------
parse_tag()
{
    local tag=""
    local hit=0
    
    #Only look for TAGS if there is a '[' at the beginning of the line
    hit=$(printf %s "$1" | sed -n -E "s/^\[.*/1/p")

    if [[ $hit == 1 ]]; then
                  
        # Try to find [ PARAM; <CONFIG_PARAM>=<VALUE> ] format tag
        tag=$( printf %s "$1" | sed -n -E "s/\[([a-zA-Z0-9_]+);([a-zA-Z0-9_]+)=([a-zA-Z0-9_]+)\]/\1;\2=\3/p")
              
        # Try to find [ SRC; <CONFIG_PARAM> ] format tag
        if [ "$tag" = "" ]; then
            tag=$( printf %s "$1" | sed -n -E "s/\[([a-zA-Z0-9_]+);([a-zA-Z0-9_]+)]/\1;\2=1/p")
        fi
        
        if [ "$tag" != "" ]; then
            CONFIG_TYPE=$( echo $tag | sed -n -E 's/([a-zA-Z0-9_]+);[a-zA-Z0-9_]+=[a-zA-Z0-9_]+/\1/p')
            CONFIG_NAME=$( echo $tag | sed -n -E 's/[a-zA-Z0-9_]+;([a-zA-Z0-9_]+)=[a-zA-Z0-9_]+/\1/p')
            CONFIG_VALUE=$( echo $tag | sed -n -E 's/[a-zA-Z0-9_]+;[a-zA-Z0-9_]+=([a-zA-Z0-9_]+)/\1/p')
        else
            error_exit "Syntax error: $1"
        fi
    fi

    return $hit
}

#---------------------------------------------------------
# This function parses the SOURCES file that contains the 
# list of source files and list of necessary compiler flags
# Arguments:
#  $1: Name of the tag type (SRC, DEF, INC, etc...)
#  $2: Name of the printer function that will be called
#      if this function finds some data to print.
#  $3: Name of the group label type
#---------------------------------------------------------
parse_sources()
{
    local CONFIG_ENABLED=true
    
    CONFIG_TYPE=""
    CONFIG_NAME=""
    CONFIG_VALUE=""

    PROPERTY_NAME=""
    PROPERTY_VALUE=""
        
    for fn in `find "$BUILD_DIR/.sources" -name 'SOURCES*'`; do
        
        #echo Processing $fn

        MODULE_NAME=""
        SOURCES_HOME=$fn

        while read ln; do

            local hit=0

            # Is it a property? (  <PROPERTY=value>  )
            #------------------------------------------
            parse_property "$ln"
            if [[ $? == 1 ]]; then
                hit=1
                eval $PROPERTY_NAME=$PROPERTY_VALUE
            fi

            # Is it a tag? (  [TYPE;TAG=value] )
            #------------------------------------
            parse_tag "$ln"
            if [[ $? == 1 ]]; then
                hit=1
                eval local my_cfg=\$$CONFIG_NAME
                if [ "$CONFIG_TYPE" = "$1" ] && [ "$my_cfg" = "$CONFIG_VALUE" ]; then
                    print_label $3 $CONFIG_NAME $MODULE_NAME
                    CONFIG_ENABLED=true
                else
                    CONFIG_ENABLED=false
                fi
            fi

            # If it is not a TAG and not a PROPERTY
            #---------------------------------------
            if [ $hit == 0 ] && [ "$ln" != "" ] && [ "$CONFIG_ENABLED" = true ]; then
                eval "$2 $ln $SOURCES_HOME"
            fi

        done < "$fn"
    done
}

#---------------------------------------------------------
# This function collects the SOURCES files to a common
# location and makes them well formatted
#---------------------------------------------------------
prepare_sources()
{
    local i=1
    local new_file

    if [ -d "$BUILD_DIR/.sources" ]; then
        rm -rf "$BUILD_DIR/.sources"
    fi
    mkdir "$BUILD_DIR/.sources"

    for srcs_path in "${SOURCES_PATH[@]}"; do
    
        srcs_path=$( cd "$srcs_path" && pwd )
        #echo Searching for SOURCES files in $srcs_path
        
        for fn in `find "$srcs_path" -name SOURCES`; do
        
            # Remove comments and whitespace from SOURCES file
            #echo Processing $fn
            new_file="$BUILD_DIR/.sources/SOURCES_$i"
            tmp_file="$BUILD_DIR/.sources/SOURCES_tmp"
            
            echo "< SOURCES_HOME = "$( cd "$( dirname "$fn" )" && pwd )" >" > $new_file
            sed -e 's/#.*$//' -e '/^$/d' ${fn} >> $new_file
            sed -e 's/[[:blank:]]*//g' < $new_file > $tmp_file
            rm $new_file
            cp $tmp_file $new_file
            i=$((i + 1))
        done
    done

    rm $tmp_file
}

#---------------------------------------------------------
# Cleanup
#---------------------------------------------------------
cleanup()
{
    if [ -d "$BUILD_DIR/.sources" ]; then
        rm -rf "$BUILD_DIR/.sources"
    fi
}

#=========================================================================================
#                                     M A I N                                             
#=========================================================================================

# Set defaults of the configuration based 
# on the config_port.h file
#----------------------------------------
parse_config_file "$SDK_PATH/porting/config_port.h"
CONFIG_HAS_SDK_CORE=1
CONFIG_HAS_LIBS=1

# Process given options and parameters
#--------------------------------------
process_opts "$@"
process_args "${@:$?}"

# Verify the configuration
#----------------------------------------
config_verification

# Print file Header
if [ $ADD_COMMENTS = true ]; then
    echo "#========================================================"
    echo "#          Auto generated file by configure.sh           "
    echo "# DO NOT MODIFY THIS FILE!!!  If you need to modify the  "
    echo "# current configuration, run the configure.sh again.     "
    echo "#--------------------------------------------------------"
    echo "# Configuration:"
    echo "# - CONFIG_OS            = $CONFIG_OS"
    echo "# - CONFIG_NETWORK_STACK = $CONFIG_NETWORK_STACK"
    echo "# - CONFIG_HAS_HTTP      = $CONFIG_HAS_HTTP"
    echo "# - CONFIG_HAS_PICOCOAP  = $CONFIG_HAS_PICOCOAP"
    echo "# - CONFIG_HAS_LIBCOAP   = $CONFIG_HAS_LIBCOAP"
    echo "# - CONFIG_SECURITY      = $CONFIG_SECURITY"
    echo "#========================================================"
    echo
fi

# prepare sources files
#------------------------
prepare_sources


if [ $SOURCE_LIST_GEN = true ]; then
    parse_sources SRC print_source_file SRC
    if [ -z "$LIB_LIST" ] && [ -n "$LIB_LIST_SRC" ]; then
        LIB_LIST=$LIB_LIST_SRC
    fi
fi

if [ $OBJECT_LIST_GEN = true ]; then
    parse_sources SRC print_object OBJ
    if [ -z "$LIB_LIST" ] && [ -n "$LIB_LIST_OBJ" ]; then
        LIB_LIST=$LIB_LIST_OBJ
    fi
fi

if [ "$LIB_MODE" = true ] && ([ $OBJECT_LIST_GEN = true ] || [ $SOURCE_LIST_GEN = true ]); then
    echo
    echo "ER_LIBRARIES = $LIB_LIST"
    echo
fi

if [ $FLAG_LIST_GEN = true ]; then
    
    parse_sources DEF print_compiler_flag DEF
    
    # Added some extra definitions
    #------------------------------
    if [ "${#EXTERNAL_DEFINES[@]}" != 0 ]; then
        print_label DEF
        for ext_def in "${EXTERNAL_DEFINES[@]}"; do
            print_compiler_flag "$ext_def"
        done
    fi
fi

if [ $INCLUDE_LIST_GEN = true ]; then
    parse_sources INC print_include_path INC
    
    # Added some extra definitions
    #------------------------------
    print_label INC
    print_include_path "$SDK_PATH"
fi

if [ $CLEAN_UP = true ]; then
    cleanup
fi

echo

exit 0
