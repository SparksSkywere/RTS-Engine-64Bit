/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
*/

bool DecompressFile(char *infile, char *outfile)
{
	(void)infile;
	(void)outfile;
	return false;
}

bool CompressFile(char *infile, char *outfile)
{
	(void)infile;
	(void)outfile;
	return false;
}

bool CompressPacket(char *inPacket, char *outPacket)
{
	(void)inPacket;
	(void)outPacket;
	return false;
}

bool DecompressPacket(char *inPacket, char *outPacket)
{
	(void)inPacket;
	(void)outPacket;
	return false;
}

unsigned int CalcNewSize(unsigned int rawSize)
{
	return rawSize;
}

bool DecompressMemory(void *inBufferVoid, int inSize, void *outBufferVoid, int& outSize)
{
	(void)inBufferVoid;
	(void)inSize;
	(void)outBufferVoid;
	outSize = 0;
	return false;
}

bool CompressMemory(void *inBufferVoid, int inSize, void *outBufferVoid, int& outSize)
{
	(void)inBufferVoid;
	(void)inSize;
	(void)outBufferVoid;
	outSize = 0;
	return false;
}
