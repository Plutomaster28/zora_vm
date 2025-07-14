#ifndef TETRA_H
#define TETRA_H

void tetra_command(int argc, char **argv);

// Subcommands
void tetra_download(int argc, char **argv);
void tetra_remove(int argc, char **argv);
void tetra_move(int argc, char **argv);
void tetra_set_location(int argc, char **argv);
void tetra_list(int argc, char **argv);

#endif // TETRA_H