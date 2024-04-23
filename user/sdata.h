typedef struct sdata{
    char *fpath;    // The path to the file we are processing
    int sencntr;    // The counter to which sentence we reached in the file

    char *lsw;      // The longest word in the current sentence
    char *lw;       // The longest word in the entire text to the current sentence (including it)
    int lw_sz;      // The length of the longest word in the entire text to the current sentence (including it)
    

    char *ssw;      // The shortest word in the current sentence
    char *sw;       // The shortest word in the entire text to the current sentence (including it)
    int sw_sz;      // The length of the shortest word in the entire text to the current sentence (including it)

    int cmd;        // Command indicator

}sdata;