#! /bin/bash

echo "Please insert password"

password=

read password

hash=`echo -n ${password} | sha512sum`

echo "char hashed_password[129] = \"${hash:0:-3}\" ;" > ./password_setup/password.c