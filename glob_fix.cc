#include <stdshit.h>
const char progName[] = "test";

int curLine;

xarray<byte> parse_line(char* str)
{
	xarray<byte> ret = {(byte*)str, 0};	
	if(!str) return ret;

	char* tok = strtok(str, " \t");
	while(tok) {
		if(tok[0] == '?') {
			int len = strlen(tok);
			memset(ret.end(), 0, len);
			ret.size += len;
		} else {
			char* end; ret.ib() = strtoul(tok, &end, 16); 
			if(tok == end) fatalError("failed to parse "
			"patch file at line: %d\n", curLine+1);
		}
	
		tok = strtok(NULL, " \t");
	}
	
	return ret;
}

struct Block
{
	char* name;
	xarray<byte> cmpData;
	xarray<short> patch;
	
	void parse(char* line);
	void apply(byte* data);
};

void Block::parse(char* line)
{
	// parse the line
	char* cmpStr = strtok(line, ",");
	char* patStr = strtok(NULL, ",");
	auto cmp = parse_line(cmpStr);
	auto pat = parse_line(patStr);

	// append patch data
	while(patch.size < cmpData.size) patch.push_back(-1);
	for(byte x : pat) {	patch.push_back(x); }
	for(byte x : cmp) { cmpData.push_back(x); }	
}

void Block::apply(byte* data)
{
	for(int x : patch) {
		if(x >= 0) *data = x;
		data++; }
}

xarray<Block> blocks;

void parse_file(cch* patchFile)
{
	int LineCount;
	char** lines = loadText(patchFile, LineCount);
	for(curLine = 0; curLine < LineCount; curLine++) {
		if(strchr(lines[curLine], '[')) { 
			blocks.push_back(lines[curLine]);
		} else {
			blocks.back().parse(lines[curLine]);
		}
	}
}

byte* sparse_find(xarray<byte> hs, xarray<byte> nd)
{
	if(hs.size < nd.size) return NULL;
	hs.size -= nd.size; 
	byte ch = nd[0]; nd = nd.right(1);
	
	for(auto& x : hs) {
		if(x != ch){ MATCH_FAIL: continue; }
		for(size_t i = 0; i < nd.size; i++) {
			if(nd[i] && nd[i] != (&x)[i+1]) 
				goto MATCH_FAIL; }
		return &x;
	}
	
	return NULL;
}

int main(int argc, char* argv[])
{
	// load the file to patch
	if(!argv[1]) fatalError("mingw glob fixer\n"
		"ussage: glob-fix <exe file> [patch file]");
	auto file = loadFile(argv[1]);
	if(!file) fatalError("failed to open file");
	
	// load patch file
	cch* patchFile = argv[2];
	if(!patchFile) patchFile  = pathCatF(
		getProgramDir(), "glob_fix.txt");
	parse_file(patchFile);
	
	for(auto& b : blocks) {
		byte* tmp = sparse_find(file, b.cmpData);
		if(tmp) { b.apply(tmp);
			if(saveFile(argv[1], file.data, file.size))
				fatalError("failed to write executable");
			return 0;
		}
	}

	fatalError("patch failed, no match");
}
