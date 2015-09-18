import os.path
import sys
import xml.dom.minidom as minidom
import struct
import array

numTables = 0

def getXpwrConfig(xml, confFile):
	numBytes = 0;
	f = open(confFile, 'wb')
	conf = minidom.parse(xml)
	node = conf.documentElement
	#parse the xpwr config
	xpwrData = conf.getElementsByTagName("xpwr_config")
	if xpwrData.length == 0:
		f.close()
		return
	xpwr = []
	for datum in xpwrData:
		elements = datum.childNodes
		for element in elements:
			xpwr.append(element)
	for top in xpwr:
		nodes = top.childNodes
		for node in nodes:
			f.write(struct.pack("B", int(node.data, 16)))
			numBytes += 1
	f.close()

	f = open(confFile, 'rb+')
	#Compute checksum and write to file
	f.seek(0)
	tmp = f.read()
	cksum = sum(array.array('B', tmp))
	f.seek(numBytes)
	f.write(struct.pack("H", cksum))
	f.close()

def getConfig(xml, confFile):
	f = open(confFile, 'wb')
	conf = minidom.parse(xml)
	node = conf.documentElement

	resOffset = 6 #header contains reserved byte after the 6th byte.
	tableOffset = 8 #The fuel gauge config table starts after the 8th byte

	#Parse the header
	headerData = conf.getElementsByTagName("header")
	head = []
	for datum in headerData:
		elements = datum.childNodes
		for element in elements:
			head.append(element)

	for top in head:
		nodes = top.childNodes
		for node in nodes:
			if node.parentNode.nodeName == "numTables":
				global numTables
				f.seek(resOffset)
				numTables = int(node.data, 16)
			format = "H" #write data as 2 byte integer
			data = struct.pack(format, int(node.data, 16))
			f.write(data)

	#Parse the fuel gauging config tables
	data = conf.getElementsByTagName("table")
	f.seek(tableOffset)
	fg = []
	for datum in data:
		elements = datum.childNodes
		for element in elements:
			fg.append(element)

	for fg_config in fg:
		nodes = fg_config.childNodes
		for node in nodes:
			try:
				if node.parentNode.nodeName == "battid" or node.parentNode.nodeName == "table_name":
					f.write(node.data) #write the characters as such
				elif node.parentNode.nodeName == "config_init" or node.parentNode.nodeName == "table_type":
					format = "B" #write data as 1 byte integer
					data = struct.pack(format, int(node.data, 16))
					f.write(data)
				else:
					format = "H" #write data as 2 byte integer
					data = struct.pack(format, int(node.data, 16))
					f.write(data)
			except Exception, e:
				print e
	f.close()

if __name__ == "__main__":
	global confFile
	basepath = os.path.dirname(__file__)
	xmlFile = str(sys.argv[1])
	confFile = str(sys.argv[2])
	fsizeOffset = 2 #header contains file size value after the 2nd byte
	cksumOffset = 4 #header contains checksum after the 4th byte

	if ("xpwr" in confFile):
		getXpwrConfig(xmlFile, confFile)
	else:
		getConfig(xmlFile, confFile)
		ip = open(confFile, 'rb+')

		#compute file size and write to the file
		ip.seek(fsizeOffset)
		format = "H"
		data = struct.pack(format, numTables * 162 + 8)
		ip.write(data)

		#compute checksum and write to the file
		ip.seek(0)
		headerBytes = ip.read()
		cksum = sum( array.array('B', headerBytes))
		format = "H"
		data = struct.pack(format, cksum)
		ip.seek(cksumOffset)
		ip.write(data)
