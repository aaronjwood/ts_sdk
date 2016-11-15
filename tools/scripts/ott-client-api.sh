#!/bin/bash
# Copyright (C) 2016 Verizon.  All rights reserved.
# Script to create and manage ThingSpace accounts and OTT devices.

# ThingSpace API URL
TS_ADDR=https://198.159.196.186

# OTT Device API URL
OTT_ADDR=https://198.159.196.124

# OTT Model ID
MODEL_ID=\"b63bb02a-a4eb-4aad-a1c3-03c7ce8926d4\"

# OTT device kind
DEV_KIND=\"ts.device.ott\"

# OTT Provider ID
PROV_ID=\"86426cac-6d0e-47ef-aa82-d149aa4fd090\"

# Basic AUTH
BAUTH=RGFrb3RhQ2xpZW50U2ltQXBwVjI6MGIxNDAwYmQtZTU1OC00NmE2LTlhNTAtYjNhMjgyYWE0ZWMz

# Info Color : Color for anything the user has to keep a record of. Red = 2
IC=2

function exit_on_error()
{
	# Exit if the response was empty or contains a word beginning with "error"
	# INPUTS:
	# $1 = Diagnostic message for the error
	# $2 = JSON output of the command for which the error is being checked
	# $3 = Return value of last shell command run (0 = no error)
	#
	# RETURNS:
	# None

	error=$(echo "$2" | egrep -o "\<error")
	if [ -n "$error" ] || [ -z "$2" ] || [ "$3" -ne "0" ]; then
		echo "$1"
		if [ -n "$2" ]; then
			echo "$2" | python -c "import json, sys; print json.load(sys.stdin)['error_description']" 2> /dev/null
		fi
		exit 1
	fi
}

function create_user_account()
{
	# INPUTS:
	# $1 = Email ID
	# $2 = Password
	#
	# RETURNS:
	# Application Access Token
	# User Access Token

	echo "Creating user account and retrieving user token"
	exit_on_error "No email provided" "$1" "0"
	exit_on_error "No password provided" "$2" "0"

	# Create an application access token in order to create an account
	echo -e "Creating application token"
	ATOKEN=$(curl -s -k -X POST \
		-H "Authorization: Basic $BAUTH" \
		-H "Content-Type: application/x-www-form-urlencoded" \
		-d 'grant_type=client_credentials&scope=ts.account.wo ts.account ts.configuration ts.dakota.inventory' \
		$TS_ADDR/oauth2/token)
	exit_on_error "There was an error in creating the client credential" "$ATOKEN" "$?"
	ATOKEN=$(echo "$ATOKEN" | python -c "import json, sys; print json.load(sys.stdin)['access_token']")
	echo "Will use the following application access token to create the user account"
	echo "Application access token (Client Token): $(tput setaf $IC)$ATOKEN$(tput sgr 0)"

	# Create the user account
	JSONOUT=$(curl -s -k -X POST \
		-H "Authorization: Bearer $ATOKEN" \
		-H "Content-Type: application/json" \
		-d "{\"email\":\""$1"\",\"password\":\""$2"\",\"noEmailVerification\":true}" \
		$TS_ADDR/api/v2/accounts)
	exit_on_error "There was a error in creating the user account" "$JSONOUT" "$?"
	echo "JSON object returned by the API:"
	echo "$JSONOUT" | python -m json.tool

	# Create a user token associated with the user account
	UTOKEN=$(curl -s -k -X POST \
		-H "Authorization: Basic $BAUTH" \
		-d "grant_type=password&scope=ts.account ts.target ts.tag ts.device ts.place ts.trigger ts.schedule ts.specification ts.specification.ro ts.data.ro ts.user&username="$1"&password="$2 \
		$TS_ADDR/oauth2/token)
	exit_on_error "Could not retrieve the user token" "$UTOKEN" "$?"
	echo "Tokens retrieved (Note down the user access token for future use):"
	UATOKEN=$(echo "$UTOKEN" | python -c "import json, sys; print json.load(sys.stdin)['access_token']")
	URTOKEN=$(echo "$UTOKEN" | python -c "import json, sys; print json.load(sys.stdin)['refresh_token']")
	echo "User access token : $(tput setaf $IC)$UATOKEN$(tput sgr 0)"
	echo "User refresh token : $(tput setaf $IC)$URTOKEN$(tput sgr 0)"

	exit 0
}

function register_ts_device()
{
	# INPUTS:
	# $1 = User Access Token
	# $2 = Device Name
	# $3 = QR Code from OTT device creation
	#
	# RETURNS:
	# ThingSpace Device ID

	echo "Registering OTT device in ThingSpace"
	exit_on_error "No user access token provided" "$1" "0"
	exit_on_error "No device name provided" "$2" "0"
	exit_on_error "No QR Code provided" "$3" "0"

	# Register the device with ThingSpace
	REGVAL=$(curl -s -k -X POST \
		-H "Authorization: Bearer $1" \
		-H "Content-Type: application/json" \
		-d "{\"kind\":"$DEV_KIND",\"name\":\""$2"\",\"providerid\":"$PROV_ID",\"qrcode\":\""$3"\"}" \
		$TS_ADDR/api/v2/devices)
	exit_on_error "Failed to register OTT device with ThingSpace" "$REGVAL" "$?"
	TSDEVID=$(echo "$REGVAL" | python -c "import json, sys; print json.load(sys.stdin)['id']")
	echo "ThingSpace device ID : $(tput setaf $IC)$TSDEVID$(tput sgr 0)"
	echo "This device should be ready to use now"

	exit 0
}

function create_ott_device()
{
	# INPUTS:
	# $1 = User Access Token
	# $2 = Device Name
	# $3 = Client Token
	#
	# RETURNS:
	# OTT QR Code
	# OTT Device ID
	# OTT Device Secret
	# ThingSpace Device ID

	echo "Creating OTT device"
	exit_on_error "No user access token provided" "$1" "0"
	exit_on_error "No device name provided" "$2" "0"
	exit_on_error "No user access token provided" "$3" "0"

	# Provision a singular OTT device in the device inventory
	PROVVAL=$(curl -s -k -X POST \
		-H "Authorization: Bearer $3" \
		-H "Content-Type: application/json" \
		-d "{\"modelId\" : "$MODEL_ID"}" \
		$OTT_ADDR/ottps/v1/devices/1)
	exit_on_error "Could not provision the device" "$PROVVAL" "$?"

	QRCODE=$(echo "$PROVVAL" | python -c "import json, sys; print json.load(sys.stdin)['devices'][0]['qrCode']" 2> /dev/null)
	exit_on_error "Could not retrieve the QR Code" "$QRCODE" "$?"
	OTTDEVID=$(echo "$PROVVAL" | python -c "import json, sys; print json.load(sys.stdin)['devices'][0]['id']" 2> /dev/null)
	exit_on_error "Could not retrieve the OTT Device ID" "$OTTDEVID" "$?"
	DEVSEC=$(echo "$PROVVAL" | python -c "import json, sys; print json.load(sys.stdin)['devices'][0]['secret']" 2> /dev/null)
	exit_on_error "Could not retrieve the Device Secret" "$DEVSEC" "$?"

	echo "Upload the device ID and device secret into the device"
	echo "QR Code : $(tput setaf $IC)$QRCODE$(tput sgr 0)"
	echo "Device ID : $(tput setaf $IC)$OTTDEVID$(tput sgr 0)"
	echo "Device secret : $(tput setaf $IC)$DEVSEC$(tput sgr 0)"

	# Register the device with ThingSpace
	register_ts_device "$1" "$2" "$QRCODE"

	exit 0
}

function list_ts_devices()
{
	# INPUTS:
	# $1 = User Access Token
	#
	# RETURNS:
	# JSON Object

	echo "Listing ThingSpace devices associated with the user account"
	exit_on_error "No user access token provided" "$1" "0"
	LISTVAL=$(curl -s -k -X GET \
		-H "Authorization: Bearer $1" \
		$TS_ADDR/api/v2/devices)
	exit_on_error "Could not retrieve the list of devices" "$LISTVAL" "$?"
	echo "JSON Object"
	echo "$LISTVAL" | python -m json.tool

	exit 0
}

function read_ott_status()
{
	# INPUTS:
	# $1 = User Access Token
	# $2 = ThingSpace Device ID
	# $3 = Number of status values to read (from the end)
	#
	# RETURNS:
	# Last 'n' OTT Status data

	echo "Reading OTT status data"
	exit_on_error "No user access token provided" "$1" "0"
	exit_on_error "No ThingSpace device ID provided" "$2" "0"
	exit_on_error "Not sure how many status values to read" "$3" "0"

	if [ $3 -ge 0 2>/dev/null ]; then
		STVAL=$(curl -s -k -X POST \
			-H "Authorization: Bearer $1" \
			-H "Content-Type: application/json" \
			$TS_ADDR/api/v2/devices/$2/fields/status/actions/history/$query{"$limitnumber":$3})
		exit_on_error "Unable to send polling interval to the device" "$STVAL" "$?"
		#echo "$STVAL" | python -c "import json, sys; print json.load(sys.stdin)[0]['fields']['status']" | base64 -D | xxd -i
		echo "$STVAL" | python -m json.tool
	else
		echo "Please enter a valid positive integer for number of statuses to read"
		exit 1
	fi

	exit 0
}

function write_ott_update()
{
	# INPUTS:
	# $1 = User Access Token
	# $2 = ThingSpace Device ID
	# $3 = Raw hexadecimal bytes (eg. 19acdc89fe)
	#
	# RETURNS:
	# JSON Object

	echo "Sending raw bytes to the device"
	exit_on_error "No user access token provided" "$1" "0"
	exit_on_error "No ThingSpace device ID provided" "$2" "0"
	exit_on_error "No data provided" "$3" "0"

	DATA=$(echo "$3" | xxd -r -p | base64)
	UPDVAL=$(curl -s -k -X POST \
		-H "Authorization: Bearer $1" \
		-H "Content-Type: application/json" \
		-d \""$DATA"\" \
		$TS_ADDR/api/v2/devices/$2/fields/update/actions/set)
	exit_on_error "Unable to send update to the device" "$UPDVAL" "$?"
	echo "New update message was queued"
	echo "$UPDVAL" | python -m json.tool

	exit 0
}

function write_ott_polling_interval()
{
	# INPUTS:
	# $1 = User Access Token
	# $2 = ThingSpace Device ID
	# $3 = Positive integer
	#
	# RETURNS:
	# JSON Object

	echo "Sending new polling interval to the device"
	exit_on_error "No user access token provided" "$1" "0"
	exit_on_error "No ThingSpace device ID provided" "$2" "0"
	exit_on_error "No polling interval provided" "$3" "0"

	if [ $3 -gt 0 2>/dev/null ]; then
		PIVAL=$(curl -s -k -X POST \
			-H "Authorization: Bearer $1" \
			-H "Content-Type: application/json" \
			-d "{\"value\" : $3}" \
			$TS_ADDR/api/v2/devices/$2/fields/pollinginterval/actions/set)
		exit_on_error "Unable to send polling interval to the device" "$PIVAL" "$?"
		PIVALSTAT=$(echo "$PIVAL" | python -c "import json, sys; print json.load(sys.stdin)['state']")
		echo "$PIVAL" | python -m json.tool
		if [ "$PIVALSTAT" != "failed" ]; then
			echo "New polling interval was queued"
		else
			echo "Unable to send polling interval to the device"
			exit 1
		fi
	else
		echo "Please enter a valid positive integer for the polling interval"
		exit 1
	fi

	exit 0
}

function write_ott_sleep_interval()
{
	# INPUTS:
	# $1 = User Access Token
	# $2 = ThingSpace Device ID
	# $3 = Positive integer
	#
	# RETURNS:
	# JSON Object

	echo "Sending new sleep interval to the device"
	exit_on_error "No user access token provided" "$1" "0"
	exit_on_error "No ThingSpace device ID provided" "$2" "0"
	exit_on_error "No sleep interval provided" "$3" "0"

	if [ $3 -gt 0 2>/dev/null ]; then
		SLPVAL=$(curl -s -k -X POST \
			-H "Authorization: Bearer $1" \
			-H "Content-Type: application/json" \
			-d "{\"value\" : $3}" \
			$TS_ADDR/api/v2/devices/$2/fields/sleep/actions/set)
		exit_on_error "Unable to send sleep interval to the device" "$SLPVAL" "$?"
		SLPVALSTAT=$(echo "$SLPVAL" | python -c "import json, sys; print json.load(sys.stdin)['state']")
		echo "$SLPVAL" | python -m json.tool
		if [ "$SLPVALSTAT" != "failed" ]; then
			echo "New sleep interval was queued"
		else
			echo "Unable to send sleep interval to the device"
			exit 1
		fi
	else
		echo "Please enter a valid positive integer for the sleep interval"
		exit 1
	fi

	exit 0
}

function revoke_application_token()
{
	# INPUTS:
	# $1 = Application Access Token
	#
	# RETURNS:
	# JSON Object

	echo "Revoke application token"
	exit_on_error "No application access token provided" "$1" "0"
	RETVAL=$(curl -w "%{http_code}" -s -k -X POST \
		-H "Authorization: Basic $BAUTH" \
		-H "Content-Type: application/x-www-form-urlencoded" \
		-d "token=$1&token_type_hint=access_token" \
		$TS_ADDR/oauth2/revoke)
	RETVAL=$(echo "$RETVAL" | egrep "^[0-9]+$")
	if [ "$RETVAL" -ne 200 ]; then
		echo "Unable to delete the token"
		exit 1
	else
		echo "Successfully deleted the application access token"
	fi

	exit 0
}

function remove_user_account()
{
	# INPUTS:
	# $1 = User Access Token
	#
	# RETURNS:
	# JSON Object

	echo "Removing user account"
	exit_on_error "No user access token provided" "$1" "0"
	RETVAL=$(curl -s -k -X DELETE \
		-H "Authorization: Bearer $1" \
		$TS_ADDR/api/v2/accounts/me)
	exit_on_error "There was an error removing the user account" "$RETVAL" "$?"
	echo "JSON object returned from the API:"
	echo "$RETVAL" | python -m json.tool

	exit 0
}

function remove_device()
{
	# INPUTS:
	# $1 = User Access Token
	# $2 = ThingSpace Device ID
	#
	# RETURNS:
	# JSON Object

	echo "Removing device from ThingSpace"
	exit_on_error "No user access token provided" "$1" "0"
	exit_on_error "No device ID provided" "$2" "0"
	REMVAL=$(curl -s -k -X DELETE \
		-H "Authorization: Bearer $1" \
		$TS_ADDR/api/v2/devices/$2)
	exit_on_error "Could not remove device from ThingSpace" "$REMVAL" "$?"
	echo "JSON object returned by the API:"
	echo "$REMVAL" | python -m json.tool

	exit 0
}

function print_usage()
{
	prg_name=$(basename "$0")
	echo "$prg_name is a script to manage ThingSpace accounts & OTT devices."
	echo -e "NOTE: Any output the user should make a note of will be colored green.\n"
	echo "Creation APIs:"
	echo "$prg_name account <e-mail> <password>"
	echo -e "\tCreate an user account. This needs to be done only once. It is"
	echo -e "\trequired to access the ThingSpace APIs. A new application token"
	echo -e "\twill automatically be created for this account. E-mail verification is"
	echo -e "\tturned off.\n"
	echo "$prg_name dev <user-access-token> <device-name>"
	echo -e "\tCreate an OTT device with the given name and register it to"
	echo -e "\tthe ThingSpace user account created using '$prg_name account (..)'.\n"
	echo "$prg_name tsdev <user-access-token> <device-name> <qr-code>"
	echo -e "\tRegister a previously provisioned OTT device with a ThingSpace"
	echo -e "\tuser account.\n\n"
	echo "Query, read and write APIs:"
	echo "$prg_name lsdev <user-access-token>"
	echo -e "\tList ThingSpace devices associated with the ThingSpace user account.\n"
	echo "$prg_name rdstat <user-access-token> <ts-device-id> <num-to-read>"
	echo -e "\tRead the last <num-to-read> status message sent by the device.\n"
	echo "$prg_name wrupd <user-access-token> <ts-device-id> <plain-hex-data>"
	echo -e "\tWrite an update message to the device. The data must be in plain"
	echo -e "\thex. Eg. 19acdc89fe\n"
	echo "$prg_name wrpi <user-access-token> <ts-device-id> <value>"
	echo -e "\tSend a new polling interval to the device.\n"
	echo "$prg_name wrsi <user-access-token> <ts-device-id> <value>"
	echo -e "\tSend a new sleep interval to the device.\n\n"
	echo -e "Revocation APIs:"
	echo "$prg_name rapptoken <application-access-token>"
	echo -e "\tRevoke ThingSpace application access token.\n"
	echo "$prg_name raccount <user-access-token>"
	echo -e "\tRemove the ThingSpace account associated with this user access token.\n"
	echo "$prg_name rmdev <user-access-token> <device-id>"
	echo -e "\tRemove the device associated with this ThingSpace user account and"
	echo -e "\tdevice ID.\n"
	exit 0
}

case "$1" in
	account)
		create_user_account "$2" "$3"
		;;
	dev)
		create_ott_device "$2" "$3" "$4"
		;;
	tsdev)
		register_ts_device "$2" "$3" "$4"
		;;
	lsdev)
		list_ts_devices "$2"
		;;
	rdstat)
		read_ott_status "$2" "$3" "$4"
		;;
	wrupd)
		write_ott_update "$2" "$3" "$4"
		;;
	wrpi)
		write_ott_polling_interval "$2" "$3" "$4"
		;;
	wrsi)
		write_ott_sleep_interval "$2" "$3" "$4"
		;;
	rapptoken)
		revoke_application_token "$2"
		;;
	raccount)
		remove_user_account "$2"
		;;
	rmdev)
		remove_device "$2" "$3"
		;;
	*)
		print_usage
		;;
esac
