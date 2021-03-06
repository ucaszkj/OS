#ifndef INCLUDE_FS_H_
#define INCLUDE_FS_H_
#include "sched.h"
#include "time.h"
#include "type.h"

#define MAX_NAME_LEN    24
#define MAX_DIR_BLK     9

//mode of file
typedef enum {
    O_RD,
    O_WR,
    O_RDWR
} file_mode_t;

//Initial Addr in MEM
#define SUPERBLK_ADDR   0xa0f00200
#define BLKMAP_ADDR     0xa0f00400
#define INODEMAP_ADDR   0xa0f04400
#define INODE_ADDR      0xa0f04600
#define DATABLK_ADDR    0xa0f44600

//Initial Addr in SD Card
#define SUPERBLK_SD_ADDR   0x20000000   //offset =512MB
#define BLKMAP_SD_ADDR     0x20000200   //16KB = 0x4000
#define INODEMAP_SD_ADDR   0x20004200   //512B = 0x200
#define INODE_SD_ADDR      0x20004400   //256KB = 0x40000
#define DATABLK_SD_ADDR    0x20044400

//SIZE
#define SECTOR_SZ   0x200               //512B
#define BLK_SZ      0x1000              //4KB

//Magic Number
#define MAGICNUM    0x46534653          //"FSFS"

//i_mode Type
#define IMODE_FILE      1
#define IMODE_DENTRY    2


//Superblock
typedef struct superblock{
    uint32_t magic_num; 
    uint32_t fs_sz; 
    uint32_t start_sector;

    uint32_t blockmap_addr;
    uint32_t blockmap_offset;
    uint32_t blockmap_num;

    uint32_t inodemap_addr;
    uint32_t inodemap_offset;
    uint32_t inodemap_num;

    uint32_t inodes_addr;
    uint32_t inodes_offset;
    uint32_t inodes_num;

    uint32_t datablock_addr;
    uint32_t datablock_offset;
    uint32_t datablock_num;

    uint32_t inode_sz;
    uint32_t dentry_sz;
} superblock_t;

//16*4=64B
#define INODE_PERBLK    64
#define INODE_SIZE      64
typedef struct inode{
    uint32_t inum;
    file_mode_t mode;               //r/w
    uint16_t  i_mode;               //0:file 1:dentry
    uint16_t  ref;                  //link num
    uint32_t  used_sz;              //used size in data block
    uint32_t  ctime;                //created time
    uint32_t  mtime;                //modified time
    uint32_t  num;                  //used num of data block or dentry 
    uint32_t direct[MAX_DIR_BLK];   //direct ptr to data block
} inode_t;

//4KB / 32B = 128
#define DENTRY_SIZE      32
typedef struct dentry {
    uint32_t  type;                 // 1: file, 2: dentry, 3:".", 4:".."
    char name[MAX_NAME_LEN];
    uint32_t inode_id;
} dentry_t;

//File Cache Ptr in MEM
superblock_t *superblock;
uint8_t *blkmap;
uint8_t *inodemap;
inode_t *inode;
inode_t *cur_inode;

//Some Buffers
dentry_t dbuf[128];                 //dentry block buffer
char file_buf[BLK_SZ/sizeof(char)]; //file block buffer
dentry_t zero_buf[128];             //zero block buffer

int init_fs();
int do_mkfs();
int do_mkdir(char *fname);
int do_rmdir(char *fname);
int do_cd(char *dir);
void do_statfs();
int do_ls(char *dir);
int do_touch(char *fname);
int do_cat(char *fname);
int do_fopen(char *fname, int access);
int do_fread(int fd, char *buff, int size);
int do_fwrite(int fd, char *buff, int size);
void do_close(int fd);

void do_fseek(int fd, int offset, int pos);
int do_rename(char *fname, char *new_name);
int do_find(char * path,char * name);

int find_path(char * dir);
void write_inode_map(uint32_t inode_id);
void write_block_inode(uint32_t inode_id);
void write_block_map(uint32_t block_id);
void init_block_map(uint32_t block_id);
uint32_t find_free_inode();
uint32_t find_free_block();
uint32_t alloc_datablock();
uint32_t alloc_inode();
void free_datablock(uint32_t block_addr);
void free_inode(uint32_t inode_id);
char * get_tail_dir(char *tail,char *dir);
char * get_head_dir(char *head,char *dir);

#endif