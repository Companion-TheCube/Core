#!/bin/bash

# Set certificate details
CERT_DIR="./"
CERT_NAME="server"
DAYS_VALID=365

# Generate private key
openssl genrsa -out "${CERT_DIR}/${CERT_NAME}.key" 2048

# Generate certificate signing request (CSR)
openssl req -new -key "${CERT_DIR}/${CERT_NAME}.key" -out "${CERT_DIR}/${CERT_NAME}.csr" \
    -subj "/C=US/ST=Georgia/L=/O=A-McD Technology LLC/CN=localhost"

# Generate self-signed certificate
openssl x509 -req -days $DAYS_VALID -in "${CERT_DIR}/${CERT_NAME}.csr" \
    -signkey "${CERT_DIR}/${CERT_NAME}.key" -out "${CERT_DIR}/${CERT_NAME}.crt"

# Clean up CSR
rm "${CERT_DIR}/${CERT_NAME}.csr"

# Create a CA bundle (concatenate the certificate and key)
cat ./server.crt ./server.key > ./ca-bundle.crt

echo "Certificate and key generated:"
echo "  ${CERT_DIR}/${CERT_NAME}.crt"
echo "  ${CERT_DIR}/${CERT_NAME}.key"