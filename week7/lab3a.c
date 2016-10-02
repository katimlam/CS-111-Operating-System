#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>

/*input output files descriptors*/
int i;
int fd;
int super_csv;
int group_csv;
int bitmap_csv;
int inode_csv;
int directory_csv;
int indirect_csv;

void errorANDexit(char* error_msg)
{
    perror(error_msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    /*open input & output file descriptors*/
    if (argc < 2)
        errorANDexit("Required argument");
    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
        errorANDexit("Cannot open input file");
    super_csv = creat("super.csv", 0644);
    group_csv = creat("group.csv", 0644);
    bitmap_csv = creat("bitmap.csv", 0644);
    inode_csv = creat("inode.csv", 0644);
    directory_csv = creat("directory.csv", 0644);
    indirect_csv = creat("indirect.csv", 0644);
    
	/*read inode structure size*/
	unsigned short inode_structure_size;
	pread(fd, &inode_structure_size, 2, 1024 + 88);
	//printf("%d", inode_structure_size);

    /*super.csv*/
    unsigned short s_magic;
    unsigned int s_inodes_count;
    unsigned int s_blocks_count;
    unsigned int s_log_block_size;
    int s_log_frag_size;
    unsigned int block_size;
    unsigned int s_blocks_per_group;
    unsigned int s_inodes_per_group;
    unsigned int s_frags_per_group;
    unsigned int s_first_data_block;
    
    pread(fd, &s_magic, 2, 1024 + 56); dprintf(super_csv, "%x,", s_magic);
    pread(fd, &s_inodes_count, 4, 1024 + 0);  dprintf(super_csv, "%d,", s_inodes_count);
    pread(fd, &s_blocks_count, 4, 1024 + 4);  dprintf(super_csv, "%d,", s_blocks_count);
    pread(fd, &s_log_block_size, 4, 1024 + 24);  dprintf(super_csv, "%d,", 1024 << s_log_block_size);
    pread(fd, &s_log_frag_size, 4, 1024 + 28);
    if (s_log_frag_size >= 0)
        block_size = (unsigned int) (1024 << s_log_frag_size);
    else
        block_size = (unsigned int)(1024 >> -(s_log_frag_size));
    dprintf(super_csv, "%d,", block_size);
    pread(fd, &s_blocks_per_group, 4, 1024 + 32);  dprintf(super_csv, "%d,", s_blocks_per_group);
    pread(fd, &s_inodes_per_group, 4, 1024 + 40);  dprintf(super_csv, "%d,", s_inodes_per_group);
    pread(fd, &s_frags_per_group, 4, 1024 + 36);  dprintf(super_csv, "%d,", s_frags_per_group);
    pread(fd, &s_first_data_block, 4, 1024 + 20);  dprintf(super_csv, "%d\n", s_first_data_block);
    
    /*group.csv*/
    struct group{
        unsigned int num_contained_blocks;
        unsigned short num_free_block;
        unsigned short num_free_inodes;
        unsigned short num_of_dir;
        unsigned int inode_bitmap_pos;
        unsigned int block_bitmap_pos;
        unsigned int inode_table_pos;
        //unsigned int num_data_blocks;
    };
    unsigned int offset_group_desc;
    float total_group;
    int total_group2;
    float left_group;
    
    if (s_first_data_block == 1)
        offset_group_desc = 2 * (1024 << s_log_block_size);
    else
        offset_group_desc = (1024 << s_log_block_size);
    
    total_group = (float)s_blocks_count / (float)s_blocks_per_group;
    total_group2 = s_blocks_count / s_blocks_per_group;
    left_group = total_group - (float)total_group2;
    int num_groups = total_group2 + 1;
    struct group group_array[num_groups];
    
    for (i = 0; i < num_groups; i++) {
        if (i == total_group2) {
            group_array[i].num_contained_blocks = (unsigned int)((float)s_blocks_per_group * left_group);
            dprintf(group_csv, "%d,", group_array[i].num_contained_blocks);
        }
        else{
            group_array[i].num_contained_blocks = s_blocks_per_group;
            dprintf(group_csv, "%d,", group_array[i].num_contained_blocks);
        }
        pread(fd, &group_array[i].num_free_block, 2, offset_group_desc + i * 32 + 12);
        dprintf(group_csv, "%d,", group_array[i].num_free_block);
        pread(fd, &group_array[i].num_free_inodes, 2, offset_group_desc + i * 32 + 14);
        dprintf(group_csv, "%d,", group_array[i].num_free_inodes);
        pread(fd, &group_array[i].num_of_dir, 2, offset_group_desc + i * 32 + 16);
        dprintf(group_csv, "%d,", group_array[i].num_of_dir);
        pread(fd, &group_array[i].inode_bitmap_pos, 4, offset_group_desc + i * 32 + 4);
        dprintf(group_csv, "%x,", group_array[i].inode_bitmap_pos);
        pread(fd, &group_array[i].block_bitmap_pos, 4, offset_group_desc + i * 32);
        dprintf(group_csv, "%x,", group_array[i].block_bitmap_pos);
        pread(fd, &group_array[i].inode_table_pos, 4, offset_group_desc + i * 32 + 8);
        dprintf(group_csv, "%x\n", group_array[i].inode_table_pos);
    }
    
    //unsigned int num_blocks_of_inode_table = s_inodes_per_group * inode_structure_size / block_size;
    
    /*bitmap.csv*/
    unsigned char my_char;
    unsigned int sum_data_blocks = 0;
    for (i = 0; i < num_groups; i++) {
        //int data_block_start_position = group_array[i].inode_table_pos + 214;
        //printf("%d\n", s_blocks_per_group);
        //printf("Group %d: %d\n", i, group_array[i].num_data_blocks);
        int k;
        int j = 0;
        unsigned char z = 0x01;
        pread(fd, &my_char, 1, group_array[i].block_bitmap_pos * block_size);
        for (k = 0 ; k < group_array[i].num_contained_blocks; k += 1)
        {
            if (k % 8 == 0){
                pread(fd, &my_char, 1, group_array[i].block_bitmap_pos * block_size + j);
                j += 1;
            }
            if ((my_char & z) == 0)
                dprintf(bitmap_csv, "%x,%d\n", group_array[i].block_bitmap_pos, sum_data_blocks + k + s_first_data_block);
            my_char >>= 1;
        }
        
        j = 0;
        for (k = 0; k < s_inodes_per_group; k += 1)
        {
            if (k % 8 == 0){
                pread(fd, &my_char, 1, group_array[i].inode_bitmap_pos * block_size + j);
                j += 1;
                z = 0x01;
            }
            if ((my_char & z) == 0){
                
                dprintf(bitmap_csv, "%x,%d\n", group_array[i].inode_bitmap_pos, i*s_inodes_per_group + k + 1);
            }
            my_char >>= 1;
        }
        sum_data_blocks += group_array[i].num_contained_blocks;
    }
    
    /*inode.csv*/
    unsigned int inode_number, ctime, mtime, atime, i_blocks, num_blocks;
    unsigned short i_mode, uid, gid, links_count;
    unsigned int i_block[15];
    int file_size;
    char file_type;
    for (i = 0; i < num_groups; i++) {
        int k = 0;
        int j = 0;
        unsigned char z = 0x01;
        for (k = 0; k < s_inodes_per_group; k += 1)
        {
            if (k % 8 == 0){
                pread(fd, &my_char, 1, group_array[i].inode_bitmap_pos * block_size + j);
                j += 1;
            }
            if ((my_char & z) == 1){
                inode_number = i*s_inodes_per_group + k + 1;
                dprintf(inode_csv, "%d,", inode_number);
                
				pread(fd, &i_mode, 2, group_array[i].inode_table_pos * block_size + k*inode_structure_size);
                if ((i_mode & 0x8000) != 0)
                    file_type = 'f';
                else if ((i_mode & 0xA000) != 0)
                    file_type = 's';
                else if ((i_mode & 0x4000) != 0)
                    file_type = 'd';
                else
                    file_type = '?';
                
                dprintf(inode_csv, "%c,", file_type);
                dprintf(inode_csv, "%o,", i_mode);
                
				pread(fd, &uid, 2, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 2); dprintf(inode_csv, "%d,", uid);
				pread(fd, &gid, 2, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 24); dprintf(inode_csv, "%d,", gid);
				pread(fd, &links_count, 2, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 26); dprintf(inode_csv, "%d,", links_count);
				pread(fd, &ctime, 4, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 12); dprintf(inode_csv, "%x,", ctime);
				pread(fd, &mtime, 4, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 16); dprintf(inode_csv, "%x,", mtime);
				pread(fd, &atime, 4, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 8); dprintf(inode_csv, "%x,", atime);
				pread(fd, &file_size, 4, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 4); dprintf(inode_csv, "%d,", file_size);
                
				pread(fd, &i_blocks, 4, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 28);
                num_blocks = i_blocks / (2 << s_log_block_size);
                dprintf(inode_csv, "%d,", num_blocks);
                
				pread(fd, &i_block, 4 * 15, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 40);
                int b;
                for (b = 0; b < 15; b++){
                    if (b != 14)
                        dprintf(inode_csv, "%x,", i_block[b]);
                    else
                        dprintf(inode_csv, "%x\n", i_block[b]);
                }
                
                //dprintf(inode_csv, "%x,%d\n", group_array[i].inode_bitmap_pos, i*s_inodes_per_group + k + 1);
            }
            my_char >>= 1;
        }
    }
    
    /*directory.csv*/
    unsigned int inode_num,offset_block;
    unsigned short rec_len;
    unsigned char name_len;
    unsigned int distance = 0;
    
    for (i = 0; i < num_groups; i++) {
        int k = 0;
        int j = 0;
        unsigned char z = 0x01;
        for (k = 0; k < s_inodes_per_group; k += 1)
        {
            unsigned int entry_num = 0;
            if (k % 8 == 0){
                pread(fd, &my_char, 1, group_array[i].inode_bitmap_pos * block_size + j);
                j += 1;
            }
            if ((my_char & z) == 1){
                inode_number = i*s_inodes_per_group + k + 1;
                
				pread(fd, &i_mode, 2, group_array[i].inode_table_pos * block_size + k*inode_structure_size);
                if ((i_mode & 0x4000) != 0){
					pread(fd, &i_block, 4 * 15, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 40);
                    int b;
                    for (b = 0; b < 15; b++){
                        if (i_block[b] != 0) {
                            pread(fd, &offset_block, 2, i_block[b]*block_size + distance + 4);
                            do
                            {
                                pread(fd, &inode_num, 4, i_block[b]*block_size + distance );
                                if (inode_num != 0 || offset_block != block_size) {
                                    if (inode_num == 0) {
                                        distance+=offset_block;
                                        entry_num++;
                                    }
                                    dprintf(directory_csv, "%d,", inode_number);
                                    dprintf(directory_csv, "%d,", entry_num);
                                    pread(fd, &rec_len, 2, i_block[b]*block_size+4 + distance);
                                    dprintf(directory_csv, "%d,", rec_len);
                                    pread(fd, &name_len, 1, i_block[b]*block_size+6 + distance);
                                    dprintf(directory_csv, "%d,", name_len);
                                    pread(fd, &inode_num, 4, i_block[b]*block_size + distance );
                                    dprintf(directory_csv, "%d,", inode_num);
                                    char *name = (char*)malloc((name_len+1)*sizeof(char));
                                    pread(fd, name, (unsigned int)name_len, i_block[b]*block_size+8 + distance);
                                    name[name_len] = '\0';
                                    dprintf(directory_csv, "\"%s\"\n", name);
                                    distance += rec_len;
                                    pread(fd, &inode_num, 4, i_block[b]*block_size + distance);
                                }
                                entry_num++;
                            }while (inode_num != 0 && distance != block_size);
                            distance = 0;
                        }
                        //dprintf(directory_csv,"%d\n\n",b);
                    }
                }
            }
            my_char >>= 1;
        }
    }
    /*indirect.csv*/
    unsigned int entry_num, block_pointer_value, indirect_block, double_indirect;
    int a,b,c;
    for (i = 0; i < num_groups; i++) {
        int k = 0;
        int j = 0;
        unsigned char z = 0x01;
        for (k = 0; k < s_inodes_per_group; k += 1)
        {
            if (k % 8 == 0){
                pread(fd, &my_char, 1, group_array[i].inode_bitmap_pos * block_size + j);
                j += 1;
            }
            if ((my_char & z) == 1){
                inode_number = i*s_inodes_per_group + k + 1;
                
				pread(fd, &i_block, 4 * 15, group_array[i].inode_table_pos * block_size + k * inode_structure_size + 40);
                entry_num = 0;
                if (i_block[12] != 0) {
                    for (a = 0; a < block_size / 4; a++) {
                        pread(fd, &block_pointer_value, 4, i_block[12]*block_size + a*4);
                        if (block_pointer_value == 0) {
                            break;
                        }
                        dprintf(indirect_csv, "%x,",i_block[12]);
                        dprintf(indirect_csv, "%d,",entry_num);
                        dprintf(indirect_csv, "%x\n",block_pointer_value);
                        entry_num++;
                    }
                }
                entry_num = 0;
                if (i_block[13] != 0) {
                    for (a = 0; a < block_size / 4; a++) {
                        pread(fd, &block_pointer_value, 4, i_block[13]*block_size + a*4);
                        if (block_pointer_value == 0) {
                            break;
                        }
                        dprintf(indirect_csv, "%x,",i_block[13]);
                        dprintf(indirect_csv, "%d,",entry_num);
                        dprintf(indirect_csv, "%x\n",block_pointer_value);
                        entry_num++;
                    }
                }
                entry_num = 0;
                // compute the indirect block inside the double-indirect
                for (a = 0; a < block_size / 4; a++) {
                    pread(fd, &block_pointer_value, 4, i_block[13]*block_size + a*4);
                    if (block_pointer_value == 0) {
                        break;
                    }
                    entry_num = 0;
                    indirect_block = block_pointer_value;
                    for (b = 0; b < block_size / 4; b++) {
                        pread(fd, &block_pointer_value, 4, indirect_block*block_size + b*4);
                        if (block_pointer_value == 0) {
                            break;
                        }
                        dprintf(indirect_csv, "%x,",indirect_block);
                        dprintf(indirect_csv, "%d,",entry_num);
                        dprintf(indirect_csv, "%x\n",block_pointer_value);
                        entry_num++;
                    }
                }
                entry_num = 0;
                //compute the indirect block inside the triple-indirect
                if (i_block[14] != 0) {
                    for (a = 0; a < block_size / 4; a++) {
                        pread(fd, &block_pointer_value, 4, i_block[14]*block_size + a*4);
                        if (block_pointer_value == 0) {
                            break;
                        }
                        dprintf(indirect_csv, "%x,",i_block[14]);
                        dprintf(indirect_csv, "%d,",entry_num);
                        dprintf(indirect_csv, "%x\n",block_pointer_value);
                        entry_num++;
                    }
                    
                }
                for (a = 0; a < block_size / 4; a++) {
                    pread(fd, &block_pointer_value, 4, i_block[14]*block_size + a*4);
                    if (block_pointer_value == 0) {
                        break;
                    }
                    entry_num = 0;
                    double_indirect = block_pointer_value;
                    for (b = 0; b < block_size / 4; b++) {
                        pread(fd, &block_pointer_value, 4, double_indirect*block_size + b*4);
                        if (block_pointer_value == 0) {
                            break;
                        }
                        dprintf(indirect_csv, "%x,",double_indirect);
                        dprintf(indirect_csv, "%d,",entry_num);
                        dprintf(indirect_csv, "%x\n",block_pointer_value);
                        entry_num++;
                    }
                }
                entry_num = 0;
                for (a = 0; a < block_size / 4; a++) {
                    pread(fd, &block_pointer_value, 4, i_block[14]*block_size + a*4);
                    if (block_pointer_value == 0) {
                        break;
                    }
                    entry_num = 0;
                    double_indirect = block_pointer_value;
                    for (b = 0; b < block_size / 4; b++) {
                        pread(fd, &block_pointer_value, 4, double_indirect*block_size + b*4);
                        if (block_pointer_value == 0) {
                            break;
                        }
                        entry_num = 0;
                        indirect_block = block_pointer_value;
                        for (c = 0; block_size / 4; c++) {
                            pread(fd, &block_pointer_value, 4, indirect_block*block_size + c*4);
                            if (block_pointer_value == 0) {
                                break;
                            }
                            dprintf(indirect_csv, "%x,",indirect_block);
                            dprintf(indirect_csv, "%d,",entry_num);
                            dprintf(indirect_csv, "%x\n",block_pointer_value);
                            entry_num++;
                        }
                    }
                }
            }
            my_char >>= 1;
        }
    }

    
    
    
    
    
    
    
    
}