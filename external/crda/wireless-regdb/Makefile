#
# This Makefile allows to build a custom regulatory database that can be used by crda.
#
# This database must be signed.
#
# If you want to build a custom database using the existing private key:
#    edit db.txt to meet your expectations
#    make regulatory.bin 
#
# If you want to build a custom database using new keys:
#   edit db.txt to meet your expectations
#   make update-keys
#   make regulatory.bin 
#

REGDB_AUTHOR = intel

REGDB_PRIVKEY ?= wireless-regdb-$(REGDB_AUTHOR).key.priv.pem
REGDB_PUBKEY ?= $(REGDB_AUTHOR).key.pub.pem

regulatory.bin: 
	@echo Generating $@ digitally signed by $(REGDB_AUTHOR)...
	@./db2bin.py regulatory.bin db.txt $(REGDB_PRIVKEY)

keys:
	@echo "Generating a new private key for $(REGDB_AUTHOR)..."
	openssl genrsa -out $(REGDB_PRIVKEY) 2048
	@echo "Generating associated public key for $(REGDB_AUTHOR)..."
	openssl rsa -in $(REGDB_PRIVKEY) -out $(REGDB_PUBKEY) -pubout -outform PEM

update-keys: keys
	@echo "Generating new crda keys-ssl.c..."
	../utils/key2pub.py --ssl $(wildcard ../pubkeys/*.pem) ../keys-ssl.c

