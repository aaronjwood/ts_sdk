#!/bin/bash
# Copyright (C) 2016 Verizon.  All rights reserved.
# Script to create and manage ThingSpace accounts and OTT devices.

prg_name=$(basename "$0")

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

# Info Color : Color for anything the user has to keep a record of. 2 = Green
IC=2

# Default config file
cfg="./.tscreds"

# Default values for credentials
user_token=""
app_cl_token=""
ts_dev=""

function read_config()
{
	while read line
	do
		if echo $line | grep -F = &>/dev/null
		then
			local fieldname=$(echo "$line" | cut -d '=' -f 1)
			local value=$(echo "$line" | cut -d '=' -f 2-)
			if [ "$fieldname" = "user_token" ]; then
				user_token=$value
			elif [ "$fieldname" = "app_cl_token" ]; then
				app_cl_token=$value
			elif [ "$fieldname" = "ts_dev" ]; then
				ts_dev=$value
			fi
		fi
	done < "$cfg"
}

function write_config()
{
	# INPUTS:
	# $1 = New user_token value
	# $2 = New app_cl_token value
	# $3 = New ts_dev value
	#
	# RETURNS:
	# None

	# Re-write old config file
	echo "user_token=$1" > "$cfg"
	echo "app_cl_token=$2" >> "$cfg"
	echo "ts_dev=$3" >> "$cfg"
}

function reset_local_profile()
{
	echo "Erasing current config file"
	echo "user_token=" > "$cfg"
	echo "app_cl_token=" >> "$cfg"
	echo "ts_dev=" >> "$cfg"
}

function exit_on_error()
{
	# Exit if the response was empty, contains a word beginning with "error",
	# or the exit code supplied is non-zero.
	# INPUTS:
	# $1 = Diagnostic message for the error
	# $2 = JSON output of the command for which the error is being checked
	# $3 = Exit code of the last shell command (0 = no error)
	#
	# RETURNS:
	# None

	local error=$(echo "$2" | egrep -o "\<error")
	if [ -n "$error" ] || [ -z "$2" ] || [ "$3" -ne "0" ]; then
		echo "$1"
		echo "$2" | python -c "import json, sys; print json.load(sys.stdin)['error_description']" 2> /dev/null
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
	exit_on_error "No email address provided" "$1" "0"
	exit_on_error "No password provided" "$2" "0"

	# Create an application access token in order to create an account
	echo -e "Creating application token"
	local ATOKEN=$(curl -s -k -X POST \
		-H "Authorization: Basic $BAUTH" \
		-H "Content-Type: application/x-www-form-urlencoded" \
		-d 'grant_type=client_credentials&scope=ts.account.wo ts.account ts.configuration ts.dakota.inventory' \
		$TS_ADDR/oauth2/token)
	exit_on_error "There was an error in creating the client credential" "$ATOKEN" "$?"
	ATOKEN=$(echo "$ATOKEN" | python -c "import json, sys; print json.load(sys.stdin)['access_token']")
	echo "Will use the following application access token to create the user account"
	echo "Application (client) access token : $(tput setaf $IC)$ATOKEN$(tput sgr 0)"

	# Create the user account
	local JSONOUT=$(curl -s -k -X POST \
		-H "Authorization: Bearer $ATOKEN" \
		-H "Content-Type: application/json" \
		-d "{\"email\":\""$1"\",\"password\":\""$2"\",\"noEmailVerification\":true}" \
		$TS_ADDR/api/v2/accounts)
	exit_on_error "There was an error in creating the user account" "$JSONOUT" "$?"
	echo "JSON object returned by the API:"
	echo "$JSONOUT" | python -m json.tool

	# Create a user token associated with the user account
	local UTOKEN=$(curl -s -k -X POST \
		-H "Authorization: Basic $BAUTH" \
		-d "grant_type=password&scope=ts.account ts.target ts.tag ts.device ts.place ts.trigger ts.schedule ts.specification ts.specification.ro ts.data.ro ts.user&username="$1"&password="$2 \
		$TS_ADDR/oauth2/token)
	exit_on_error "Could not retrieve the user token" "$UTOKEN" "$?"
	echo "Tokens retrieved (Note down the user access token for future use):"
	local UATOKEN=$(echo "$UTOKEN" | python -c "import json, sys; print json.load(sys.stdin)['access_token']")
	local URTOKEN=$(echo "$UTOKEN" | python -c "import json, sys; print json.load(sys.stdin)['refresh_token']")
	echo "User access token : $(tput setaf $IC)$UATOKEN$(tput sgr 0)"
	echo "User refresh token : $(tput setaf $IC)$URTOKEN$(tput sgr 0)"

	echo "Writing tokens to config file ($cfg)"
	write_config "$UATOKEN" "$ATOKEN" "$ts_dev"

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
	local REGVAL=$(curl -s -k -X POST \
		-H "Authorization: Bearer $1" \
		-H "Content-Type: application/json" \
		-d "{\"kind\":"$DEV_KIND",\"name\":\""$2"\",\"providerid\":"$PROV_ID",\"qrcode\":\""$3"\"}" \
		$TS_ADDR/api/v2/devices)
	exit_on_error "Failed to register OTT device with ThingSpace" "$REGVAL" "$?"
	local TSDEVID=$(echo "$REGVAL" | python -c "import json, sys; print json.load(sys.stdin)['id']")
	echo "ThingSpace device ID : $(tput setaf $IC)$TSDEVID$(tput sgr 0)"
	echo "This device should be ready to use now"

	echo "Writing device to config file ("$cfg")"
	write_config "$user_token" "$app_cl_token" "$TSDEVID"

	exit 0
}

function create_ott_device()
{
	# INPUTS:
	# $1 = Device Name
	# $2 = QR Code
	# $3 = User Access Token
	# $4 = Application (client) access token
	#
	# RETURNS:
	# OTT QR Code
	# OTT Device ID
	# OTT Device Secret
	# ThingSpace Device ID

	local DEVNAME="$1"
	local QRCODE="$2"
	local UATOKEN="${3:-$user_token}"
	local APPTOKEN="${4:-$app_cl_token}"

	echo "Creating OTT device"
	exit_on_error "No device name provided" "$DEVNAME" "0"
	exit_on_error "No application (client) access token provided" "$APPTOKEN" "0"
	exit_on_error "No user access token provided" "$UATOKEN" "0"

	if [ -z "$QRCODE" ]; then	# No QR Code provided, so generate one
		# Provision a singular OTT device in the device inventory
		local PROVVAL=$(curl -s -k -X POST \
			-H "Authorization: Bearer $APPTOKEN" \
			-H "Content-Type: application/json" \
			-d "{\"modelId\" : "$MODEL_ID"}" \
			$OTT_ADDR/ottps/v1/devices/1)
		exit_on_error "Could not provision the device" "$PROVVAL" "$?"

		# XXX: NGINX is unstable here, might return Service Unavailable instead of JSON, hence the redirection to /dev/null
		QRCODE=$(echo "$PROVVAL" | python -c "import json, sys; print json.load(sys.stdin)['devices'][0]['qrCode']" 2> /dev/null)
		exit_on_error "Could not retrieve the QR Code" "$QRCODE" "$?"
		local OTTDEVID=$(echo "$PROVVAL" | python -c "import json, sys; print json.load(sys.stdin)['devices'][0]['id']" 2> /dev/null)
		exit_on_error "Could not retrieve the OTT Device ID" "$OTTDEVID" "$?"
		local DEVSEC=$(echo "$PROVVAL" | python -c "import json, sys; print json.load(sys.stdin)['devices'][0]['secret']" 2> /dev/null)
		exit_on_error "Could not retrieve the Device Secret" "$DEVSEC" "$?"

		echo "QR Code : $(tput setaf $IC)$QRCODE$(tput sgr 0)"
		echo "Device ID : $(tput setaf $IC)$OTTDEVID$(tput sgr 0)"
		echo "Device secret : $(tput setaf $IC)$DEVSEC$(tput sgr 0)"
		echo -e "Device ID (C compatible) :\n$(echo $OTTDEVID | xxd -r -p | xxd -i -c 8)"
		echo -e "Device secret (C compatible) :\n$(echo $DEVSEC | base64 -D | xxd -i -c 8)"
	fi

	# Register the device with ThingSpace
	register_ts_device "$UATOKEN" "$DEVNAME" "$QRCODE"

	exit 0
}

function list_ts_devices()
{
	# INPUTS:
	# $1 = User Access Token
	#
	# RETURNS:
	# JSON Object

	local UATOKEN=${1:-$user_token}
	echo "Listing ThingSpace devices associated with the user account"
	exit_on_error "No user access token provided" "$UATOKEN" "0"
	local LISTVAL=$(curl -s -k -X GET \
		-H "Authorization: Bearer $UATOKEN" \
		$TS_ADDR/api/v2/devices)
	exit_on_error "Could not retrieve the list of devices" "$LISTVAL" "$?"
	echo "JSON Object"
	echo "$LISTVAL" | python -m json.tool

	exit 0
}

function read_ott_status()
{
	# INPUTS:
	# $1 = Number of status values to read (latest status first)
	# $2 = ThingSpace Device ID
	# $3 = User Access Token
	#
	# RETURNS:
	# Last 'n' OTT Status data

	local NUMSTAT=$1
	local TSDEVID=${2:-$ts_dev}
	local UATOKEN=${3:-$user_token}

	echo "Reading OTT status data"
	exit_on_error "Not sure how many status values to read" "$NUMSTAT" "0"
	exit_on_error "No user access token provided" "$UATOKEN" "0"
	exit_on_error "No ThingSpace device ID provided" "$TSDEVID" "0"

	if [ "$NUMSTAT" -gt 0 2>/dev/null ]; then
		local STVAL=$(curl -s -k -X POST \
			-H "Authorization: Bearer $UATOKEN" \
			-H "Content-Type: application/json" \
			-d "{\"\$limitnumber\" : $NUMSTAT}" \
			$TS_ADDR/api/v2/devices/$TSDEVID/fields/status/actions/history)
		exit_on_error "Unable to read the device status" "$STVAL" "$?"
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
	# $1 = Raw hexadecimal bytes (eg. 19acdc89fe)
	# $2 = ThingSpace Device ID
	# $3 = User Access Token
	#
	# RETURNS:
	# JSON Object

	local HEXDATA=$1
	local TSDEVID=${2:-$ts_dev}
	local UATOKEN=${3:-$user_token}

	echo "Sending raw bytes to the device"
	exit_on_error "No data provided" "$HEXDATA" "0"
	exit_on_error "No ThingSpace device ID provided" "$TSDEVID" "0"
	exit_on_error "No user access token provided" "$UATOKEN" "0"

	local DATA=$(echo "$HEXDATA" | xxd -r -p | base64)
	local UPDVAL=$(curl -s -k -X POST \
		-H "Authorization: Bearer $UATOKEN" \
		-H "Content-Type: application/json" \
		-d \""$DATA"\" \
		$TS_ADDR/api/v2/devices/$TSDEVID/fields/update/actions/set)
	exit_on_error "Unable to send update to the device" "$UPDVAL" "$?"
	echo "New update message was queued"
	echo "$UPDVAL" | python -m json.tool

	exit 0
}

function write_ott_polling_interval()
{
	# INPUTS:
	# $1 = Positive integer
	# $2 = ThingSpace Device ID
	# $3 = User Access Token
	#
	# RETURNS:
	# JSON Object

	local VALUE=$1
	local TSDEVID=${2:-$ts_dev}
	local UATOKEN=${3:-$user_token}

	echo "Sending new polling interval to the device"
	exit_on_error "No polling interval provided" "$VALUE" "0"
	exit_on_error "No ThingSpace device ID provided" "$TSDEVID" "0"
	exit_on_error "No user access token provided" "$UATOKEN" "0"

	if [ "$VALUE" -gt 0 2>/dev/null ]; then
		local PIVAL=$(curl -s -k -X POST \
			-H "Authorization: Bearer $UATOKEN" \
			-H "Content-Type: application/json" \
			-d "{\"value\" : $VALUE}" \
			$TS_ADDR/api/v2/devices/$TSDEVID/fields/pollinginterval/actions/set)
		exit_on_error "Unable to send polling interval to the device" "$PIVAL" "$?"
		local PIVALSTAT=$(echo "$PIVAL" | python -c "import json, sys; print json.load(sys.stdin)['state']")
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
	# $1 = Positive integer
	# $2 = ThingSpace Device ID
	# $3 = User Access Token
	#
	# RETURNS:
	# JSON Object

	local VALUE=$1
	local TSDEVID=${2:-$ts_dev}
	local UATOKEN=${3:-$user_token}

	echo "Sending new sleep interval to the device"
	exit_on_error "No sleep interval provided" "$VALUE" "0"
	exit_on_error "No ThingSpace device ID provided" "$TSDEVID" "0"
	exit_on_error "No user access token provided" "$UATOKEN" "0"

	if [ "$VALUE" -gt 0 2>/dev/null ]; then
		local SLPVAL=$(curl -s -k -X POST \
			-H "Authorization: Bearer $UATOKEN" \
			-H "Content-Type: application/json" \
			-d "{\"value\" : $VALUE}" \
			$TS_ADDR/api/v2/devices/$TSDEVID/fields/sleep/actions/set)
		exit_on_error "Unable to send sleep interval to the device" "$SLPVAL" "$?"
		local SLPVALSTAT=$(echo "$SLPVAL" | python -c "import json, sys; print json.load(sys.stdin)['state']")
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
	# None

	echo "Revoke application token"
	exit_on_error "No application access token provided" "$1" "0"
	local RETVAL=$(curl -w "%{http_code}" -s -k -X POST \
		-H "Authorization: Basic $BAUTH" \
		-H "Content-Type: application/x-www-form-urlencoded" \
		-d "token=$1&token_type_hint=access_token" \
		$TS_ADDR/oauth2/revoke)
	local EXITCODE=$(echo "$RETVAL" | egrep "^[0-9]+$")
	if [ "$EXITCODE" -ne 200 ]; then
		echo "Unable to delete the token"
		exit 1
	else
		echo "Successfully deleted the application access token"
		echo "Removing stale values from the config file"

		write_config "$user_token" "" "$ts_dev"
	fi

	exit 0
}

function remove_user_account()
{
	# INPUTS:
	# $1 = User Access Token
	#
	# RETURNS:
	# None

	echo "Removing user account"
	exit_on_error "No user access token provided" "$1" "0"
	local RETVAL=$(curl -w "%{http_code}" -s -k -X DELETE \
		-H "Authorization: Bearer $1" \
		$TS_ADDR/api/v2/accounts/me)
	local EXITCODE=$(echo "$RETVAL" | egrep "^[0-9]+$")
	if [ "$EXITCODE" -ne 204 ]; then
		# Filter out the HTTP return code
		RETVAL=$(echo "$RETVAL" | head -n 1)
		exit_on_error "Unable to delete user account" "$RETVAL" "0"
		exit 1
	else
		echo "Successfully deleted the user account"
		echo "Removing stale values from the config file"

		write_config "" "$app_cl_token" "$ts_dev"
	fi

	exit 0
}

function remove_device()
{
	# INPUTS:
	# $1 = User Access Token
	# $2 = ThingSpace Device ID
	#
	# RETURNS:
	# None

	echo "Removing device from ThingSpace"
	exit_on_error "No user access token provided" "$1" "0"
	exit_on_error "No device ID provided" "$2" "0"
	local RETVAL=$(curl -w "%{http_code}" -s -k -X DELETE \
		-H "Authorization: Bearer $1" \
		$TS_ADDR/api/v2/devices/$2)
	local EXITCODE=$(echo "$RETVAL" | egrep "^[0-9]+$")
	if [ "$EXITCODE" -ne 204 ]; then
		# Filter out the HTTP return code
		local RETVAL=$(echo "$RETVAL" | head -n 1)
		exit_on_error "Could not remove device from ThingSpace" "$RETVAL" "0"
		exit 1
	else
		echo "Successfully deleted the device"
		echo "Removing stale values from the config file"

		write_config "$user_token" "$app_cl_token" ""
	fi

	exit 0
}

function print_usage_small()
{
	echo "$prg_name is a script to manage ThingSpace accounts & OTT devices."
	echo "Copyright (C) 2016 Verizon. All rights reserved."
	echo "NOTE: Any output the user should note down will be colored green."
}

function print_usage()
{
	less << EOF
$prg_name is a script to manage ThingSpace accounts & OTT devices.
Copyright (C) 2016 Verizon. All rights reserved.
NOTE: Any output the user should note down will be colored green.
If an optional parameter is not provided, it will be read from the conf file : $cfg

Creation APIs:
$prg_name account <e-mail> <password>
	Create an user account. This needs to be done only once. It is required
	to access the ThingSpace APIs. A new application token will be automatically
	created for this account. E-mail verification is turned off.

$prg_name dev <device-name> [qr-code] [user-access-token] [app-cl-token]
	Create an OTT device with the given name and register it to the
	ThingSpace user account. If a QR code is provided, register the previously
	provisioned OTT device.


Query APIs:
$prg_name lsdev [user-access-token]
	List ThingSpace devices associated with the ThingSpace user account.


Read and write APIs:
$prg_name rdstat <num-to-read> [ts-device-id] [user-access-token]
	Read at most the last <num-to-read> status message sent by the device.

$prg_name wrupd <plain-hex-data> [ts-device-id] [user-access-token]
	Write an update message to the device. The data must be in plain
	hex. Eg. 19acdc89fe

$prg_name wrpi <value> [ts-device-id] [user-access-token]
	Send a new polling interval to the device.

$prg_name wrsi <value> [ts-device-id] [user-access-token]
	Send a new sleep interval to the device.


Revocation APIs:
$prg_name rapptoken <application-access-token>
	Revoke ThingSpace application (client) access token.

$prg_name raccount <user-access-token>
	Remove the ThingSpace account associated with this user access token.
	Accounts cannot be removed if there are devices associated with it.

$prg_name rmdev <user-access-token> <device-id>
	Remove the device associated with this ThingSpace user account and
	device ID.


Utility functions:
$prg_name reset_prof
	Reset the local profile (./.tscred). Populate this file
	with values of application (client) access token, user access
	token and device ID. If a parameter related to one of these is
	enclosed in '[' and ']' and is omitted from the command line,
	the program will use the value from the file.
EOF
	exit 0
}

if [ -f "$cfg" ]; then
	read_config
else
	reset_local_profile
fi

case "$1" in
	reset_prof)
		reset_local_profile
		;;
	account)
		create_user_account "$2" "$3"
		;;
	dev)
		create_ott_device "$2" "$3" "$4" "$5"
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
	help)
		print_usage
		;;
	*)
		print_usage_small
		echo "Type in '$prg_name help' for more information."
		;;
esac
