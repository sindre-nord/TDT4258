#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <linux/input.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <poll.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h> // Needed for mmap
#include <linux/fb.h>
#include <limits.h> // Needed for PATH_MAX
// Needed to fine the sense hat framebuffer
#include <dirent.h>
#include <fnmatch.h>


// The game state can be used to detect what happens on the playfield
#define GAMEOVER   0
#define ACTIVE     (1 << 0)
#define ROW_CLEAR  (1 << 1)
#define TILE_ADDED (1 << 2)

// If you extend this structure, either avoid pointers or adjust
// the game logic allocate/deallocate and reset the memory
typedef struct {
  bool occupied;
} tile;

typedef struct {
  unsigned int x;
  unsigned int y;
} coord;

typedef struct {
  coord const grid;                     // playfield bounds
  unsigned long const uSecTickTime;     // tick rate
  unsigned long const rowsPerLevel;     // speed up after clearing rows
  unsigned long const initNextGameTick; // initial value of nextGameTick

  unsigned int tiles; // number of tiles played
  unsigned int rows;  // number of rows cleared
  unsigned int score; // game score
  unsigned int level; // game level

  tile *rawPlayfield; // pointer to raw memory of the playfield
  tile **playfield;   // This is the play field array
  unsigned int state;
  coord activeTile;                       // current tile

  unsigned long tick;         // incremeted at tickrate, wraps at nextGameTick
                              // when reached 0, next game state calculated
  unsigned long nextGameTick; // sets when tick is wrapping back to zero
                              // lowers with increasing level, never reaches 0
} gameConfig;


typedef struct {
  // 5 bits for red, 6 bits for green and 5 bits for blue
  unsigned int red;
  unsigned int green;
  unsigned int blue;
} color;

typedef struct {
  unsigned int col;
  unsigned int row;
  // Color is RGB with 5 bits for red, 6 bits for green and 5 bits for blue
  color color;
} pixel;



gameConfig game = {
                   .grid = {8, 8},
                   .uSecTickTime = 10000,
                   .rowsPerLevel = 2,
                   .initNextGameTick = 50,
};


#define SENSE_HAT_JOYSTICK_ID "Raspberry Pi Sense HAT Joystick"
int is_input_sense_hat_joystick(int local_joystick) {
    // Check if the input device is the sense hat joystick
    // Return 1 if it is
    // Return 0 if there is an error or it's not the joystick
    char name[256] = "unknown";
    
    // Using ioctl to get the name of the input device
    // Input output control takes the file descriptor, the request, and a pointer to the data
    // The Event Input Ouput Control Get Name request, the EVIOCGNAME macro, gets the name of the input device
    // https://www.linuxjournal.com/article/6429
    if (ioctl(local_joystick, EVIOCGNAME(sizeof(name)), name) < 0) {
        perror("Error reading input device name");
        close(local_joystick);
        return 0;
    }

    if (!strcmp(name, SENSE_HAT_JOYSTICK_ID)) {
        return 1;
    }

    return 0;
}

int initializeJoystick() {
  // Joystick will be on one of the /dev/input/event* files
  DIR *dir;
  struct dirent *entry;
  char *joystick_pattern = "event*";
  char *joystick_path = "/dev/input/";

  dir = opendir(joystick_path);
  if (!dir) {
    perror("Unable to open directory");
    return false;
  }
  while ((entry = readdir(dir)) != NULL){
    if (fnmatch(joystick_pattern, entry->d_name, 0) == 0) {
      //printf("Found joystick: %s\n", entry->d_name);
      char full_path[PATH_MAX];
      const char format[] = "%s/%s";
      snprintf(full_path, sizeof(full_path), format, joystick_path, entry->d_name);
      int local_joystick = open(full_path, O_RDONLY | O_NONBLOCK);
      if (local_joystick == -1) {
        perror("Error opening joystick device");
        closedir(dir);
        return false;
      }
      if (is_input_sense_hat_joystick(local_joystick)){
        closedir(dir);
        //printf("Found sense hat joystick: %s\n", entry->d_name);
        return local_joystick;
      } else {
        close(local_joystick);
      }
    }
  }

  return true;
}

#define SENSE_HAT_FB_ID "RPi-Sense FB"
#define SENSE_HAT_FB_PATH "/dev"
#define SENSE_HAT_FB_PATTERN "fb*"
u_int16_t *pixel_grid; // Maps to memory
color *color_grid; // Keeps dem colors
char *fbdatamap;
int fbdatasize;
int fb;
int joystick;

color colors[7] = {
    {0, 63, 31},     // Cyan
    {31, 63, 0},     // Yellow
    {31, 15, 31},    // Purple
    {0, 63, 0},      // Green
    {31, 0, 0},      // Red
    {0, 0, 31},      // Blue
    {31, 31, 0}      // Orange
};
int color_iterator = 0;



// Checks the id of the framebuffer to see if it is the sense hat framebuffer
// returns 1 if it is, 0 if it is not
int is_fb_sense_hat(int local_fb){
    // Check if the framebuffer is the sense hat framebuffer
    // Return 1 if it is
    // Return 0 if there is an error
    struct fb_fix_screeninfo finfo;
    if (ioctl(local_fb, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        close(local_fb);
        return 0;
    }
    if (!strcmp(finfo.id, SENSE_HAT_FB_ID)) {
        return 1;
    }
    return 0;
}

// Function returns an fb object on sucess and -1 on failure
int find_frame_buffer_by_id(char *id){
    // Sear for all framebuffers
    // If id matches, return the file descriptor
    // Else return -1
    DIR *dir;
    struct dirent *entry;
    const char *directory_path = "/dev"; // default to /dev directory
    const char *pattern = "fb*"; // The pattern to match

    dir = opendir(directory_path);
    if (!dir) {
        perror("Unable to open directory");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        // printf("Found entry: %s\n", entry->d_name);
        if (fnmatch(pattern, entry->d_name, 0) == 0) { // This is true if we have a match
            //printf("Found framebuffer: %s\n", entry->d_name);
            char full_path[PATH_MAX];
            const char format[] = "%s/%s";
            snprintf(full_path, sizeof(full_path), format, directory_path, entry->d_name);
            int local_fb = open(full_path, O_RDWR);
            if (local_fb == -1) {
                perror("Error opening framebuffer device");
                closedir(dir);
                return -1;
            }
            if (is_fb_sense_hat(local_fb)){
                closedir(dir);
                //printf("Found sense hat framebuffer: %s\n", entry->d_name);
                return local_fb;
            } else {
                close(local_fb);
            }

        }
    }
    closedir(dir);
    return -1;
}
// This function is called on the start of your application
// Here you can initialize what ever you need for your task
// return false if something fails, else true
bool initializeSenseHat() {
  // Initialize the joystick
  joystick = initializeJoystick();
  if (!joystick){
    return false;
  }


  // There shoud be a fb0 in the /dev directory, which is the framebuffer
  // of the LED matrix. You can open it and write to it like a file.

  fb = find_frame_buffer_by_id(SENSE_HAT_FB_ID);
  if (!fb) {
    return false;
  }
  struct fb_var_screeninfo vinfo;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fb);
        return 1;
    }
    struct fb_fix_screeninfo finfo;
    if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        close(fb);
        return 1;
    }
    // print_sense_hat_info(fb);

    // Map the data to memory, first calculate the size
    fbdatasize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    fbdatamap = mmap(0, fbdatasize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (fbdatamap == MAP_FAILED) {
        perror("mmap failed");
        close(fb);
        return 1;
    }
    // Turn on some of the LEDs on the sense hat:
    // The sense hat has 64 LEDs
    // Each LED has 3 colors: red, green, blue
    // Each color has 8 levels of brightness
    // The color is set by writing to the framebuffer
    // The framebuffer is a 2D array of pixels
    // Each pixel is 16 bits
    // The first 5 bits are for the red color
    // The next 6 bits are for the green color
    // The last 5 bits are for the blue color

    // Clear the whole thing:
    memset(fbdatamap, 0, fbdatasize);

    // Set the first LED to red:
    // The first LED is at position 0, 0
    // The first pixel is at position 0, 0
    pixel_grid = (u_int16_t *)fbdatamap;
    color_grid = (color *)malloc(sizeof(color) * vinfo.xres * vinfo.yres);
    memset(color_grid, 0, sizeof(color) * vinfo.xres * vinfo.yres); // Clear the color grid

  return true;
}

// This function is called when the application exits
// Here you can free up everything that you might have opened/allocated
void freeSenseHat() {
  // Turn off all LEDs
  memset(fbdatamap, 0, fbdatasize);
  munmap(fbdatamap, fbdatasize);
  // Release the color grid
  free(color_grid);
  close(fb);
}


#include <poll.h>
// This function should return the key that corresponds to the joystick press
// KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, with the respective direction
// and KEY_ENTER, when the the joystick is pressed
// !!! when nothing was pressed you MUST return 0 !!!
int readSenseHatJoystick() {
  struct pollfd pollJoystick = {
       .fd = joystick, // File descriptor
       .events = POLLIN // Requested event
  };
  int pollingTimeout = 10; // Timeout in ms

  // Perform an evaluate the actual poll
  int pollingResult = poll(&pollJoystick, 1, pollingTimeout);
  // Check the result
  if (pollingResult == -1) {
      // This is bad, but im not supposed to alter behavior so who knows how to deal with
      // errors...
      return 0;
  }
  if (pollingResult == 0) {
      return 0;
  }
  if (!(pollJoystick.revents & POLLIN)) {
      // This not happening means we gucci.
      return 0;
  }


  // Joystick 
  struct input_event ev;
  ssize_t n = read(joystick, &ev, sizeof(ev));

  if (n == (ssize_t)sizeof(ev)) {
      if (ev.type == EV_KEY && ev.value == 1) {
        if (ev.code == KEY_UP || ev.code == KEY_DOWN || ev.code == KEY_LEFT || 
            ev.code == KEY_RIGHT || ev.code == KEY_ENTER) {
            //printf("Joystick pressed: %d\n", ev.code);
            return ev.code;
        }
      }
  }
  return 0;
}

void render_pixel(pixel pix){
  pixel_grid[pix.row * 8 + pix.col] = (pix.color.red << 11) | (pix.color.green << 5) | pix.color.blue; // Bitmask my man  
}

// This function should render the gamefield on the LED matrix. It is called
// every game tick. The parameter playfieldChanged signals whether the game logic
// has changed the playfield
void renderSenseHatMatrix(bool const playfieldChanged) {
  if (!playfieldChanged){
    return;
  }
  // // For demo only
  // for(int i=0; i<64; i++){
  //   pixel pix;
  //   pix.row = i / 8;
  //   pix.col = i % 8;
  //   pix.color.red = i*2;
  //   pix.color.green = i*2;
  //   pix.color.blue = i*3;
  //   render_pixel(pix);
  // }
  // Render based on the game grid and blocks instead:
  // 1. Clear the whole thing:
  memset(fbdatamap, 0, fbdatasize);
  // 2. Render the blocks:
  for(int i=0; i<game.grid.x * game.grid.y; i++){
    pixel pix;
    pix.row = i / 8;
    pix.col = i % 8;
    pix.color = color_grid[i];
    render_pixel(pix);
  }

  //(void) playfieldChanged;
}


// The game logic uses only the following functions to interact with the playfield.
// if you choose to change the playfield or the tile structure, you might need to
// adjust this game logic <> playfield interface

static inline void newTile(coord const target) {
  game.playfield[target.y][target.x].occupied = true;
}

static inline void copyTile(coord const to, coord const from) {
  memcpy((void *) &game.playfield[to.y][to.x], (void *) &game.playfield[from.y][from.x], sizeof(tile));
}

static inline void copyRow(unsigned int const to, unsigned int const from) {
  memcpy((void *) &game.playfield[to][0], (void *) &game.playfield[from][0], sizeof(tile) * game.grid.x);

}

static inline void resetTile(coord const target) {
  memset((void *) &game.playfield[target.y][target.x], 0, sizeof(tile));
}

static inline void resetRow(unsigned int const target) {
  memset((void *) &game.playfield[target][0], 0, sizeof(tile) * game.grid.x);
}

static inline bool tileOccupied(coord const target) {
  return game.playfield[target.y][target.x].occupied;
}

static inline bool rowOccupied(unsigned int const target) {
  for (unsigned int x = 0; x < game.grid.x; x++) {
    coord const checkTile = {x, target};
    if (!tileOccupied(checkTile)) {
      return false;
    }
  }
  return true;
}


static inline void resetPlayfield() {
  for (unsigned int y = 0; y < game.grid.y; y++) {
    resetRow(y);
  }
}

// Below here comes the game logic. Keep in mind: You are not allowed to change how the game works!
// that means no changes are necessary below this line! And if you choose to change something
// keep it compatible with what was provided to you!

bool addNewTile() {
  game.activeTile.y = 0;
  game.activeTile.x = (game.grid.x - 1) / 2;
  if (tileOccupied(game.activeTile))
    return false;
  newTile(game.activeTile);
  // Also make give a color to the new tile
  color_grid[game.activeTile.y * game.grid.x + game.activeTile.x] = colors[color_iterator];
  color_iterator++;
  if (color_iterator >= 7){
    color_iterator = 0;
  }
  return true;
}

bool moveRight() {
  coord const newTile = {game.activeTile.x + 1, game.activeTile.y};
  if (game.activeTile.x < (game.grid.x - 1) && !tileOccupied(newTile)) {
    copyTile(newTile, game.activeTile);
    resetTile(game.activeTile);
    game.activeTile = newTile;
    // Also move the color
    color_grid[game.activeTile.y * game.grid.x + game.activeTile.x] = color_grid[(game.activeTile.y) * game.grid.x + (game.activeTile.x - 1)];
    color_grid[(game.activeTile.y) * game.grid.x + (game.activeTile.x - 1)] = (color){0, 0, 0};
    return true;
  }
  return false;
}

bool moveLeft() {
  coord const newTile = {game.activeTile.x - 1, game.activeTile.y};
  if (game.activeTile.x > 0 && !tileOccupied(newTile)) {
    copyTile(newTile, game.activeTile);
    resetTile(game.activeTile);
    game.activeTile = newTile;
    // Also move the color
    color_grid[game.activeTile.y * game.grid.x + game.activeTile.x] = color_grid[(game.activeTile.y) * game.grid.x + (game.activeTile.x + 1)];
    color_grid[(game.activeTile.y) * game.grid.x + (game.activeTile.x + 1)] = (color){0, 0, 0};

    return true;
  }
  return false;
}


bool moveDown() {
  coord const newTile = {game.activeTile.x, game.activeTile.y + 1};
  if (game.activeTile.y < (game.grid.y - 1) && !tileOccupied(newTile)) {
    copyTile(newTile, game.activeTile);
    resetTile(game.activeTile);
    game.activeTile = newTile;
    // Also move the color
    color_grid[game.activeTile.y * game.grid.x + game.activeTile.x] = color_grid[(game.activeTile.y - 1) * game.grid.x + (game.activeTile.x)];
    color_grid[(game.activeTile.y - 1) * game.grid.x + (game.activeTile.x)] = (color){0, 0, 0};
  
    return true;
  }
  return false;
}


bool clearRow() {
  if (rowOccupied(game.grid.y - 1)) {
    for (unsigned int y = game.grid.y - 1; y > 0; y--) {
      copyRow(y, y - 1);
    }
    resetRow(0);
    // Also move the colors
    for (unsigned int y = game.grid.y - 1; y > 0; y--) {
      for (unsigned int x = 0; x < game.grid.x; x++) {
        color_grid[y * game.grid.x + x] = color_grid[(y - 1) * game.grid.x + x];
        color_grid[(y - 1) * game.grid.x + x] = (color){0, 0, 0};
      }
    }

    return true;
  }
  return false;
}

void advanceLevel() {
  game.level++;
  switch(game.nextGameTick) {
  case 1:
    break;
  case 2 ... 10:
    game.nextGameTick--;
    break;
  case 11 ... 20:
    game.nextGameTick -= 2;
    break;
  default:
    game.nextGameTick -= 10;
  }
}

void newGame() {
  game.state = ACTIVE;
  game.tiles = 0;
  game.rows = 0;
  game.score = 0;
  game.tick = 0;
  game.level = 0;
  resetPlayfield();
  // Clear the sense hat matrix
  memset(pixel_grid, 0, sizeof(u_int16_t) * 64);
  // Clear the colors
  memset(color_grid, 0, sizeof(color) * 64);
}

void gameOver() {
  game.state = GAMEOVER;
  game.nextGameTick = game.initNextGameTick;
}


bool sTetris(int const key) {
  bool playfieldChanged = false;

  if (game.state & ACTIVE) {
    // Move the current tile
    if (key) {
      playfieldChanged = true;
      switch(key) {
      case KEY_LEFT:
        moveLeft();
        break;
      case KEY_RIGHT:
        moveRight();
        break;
      case KEY_DOWN:
        while (moveDown()) {};
        game.tick = 0;
        break;
      default:
        playfieldChanged = false;
      }
    }

    // If we have reached a tick to update the game
    if (game.tick == 0) {
      // We communicate the row clear and tile add over the game state
      // clear these bits if they were set before
      game.state &= ~(ROW_CLEAR | TILE_ADDED);

      playfieldChanged = true;
      // Clear row if possible
      if (clearRow()) {
        game.state |= ROW_CLEAR;
        game.rows++;
        game.score += game.level + 1;
        if ((game.rows % game.rowsPerLevel) == 0) {
          advanceLevel();
        }
      }

      // if there is no current tile or we cannot move it down,
      // add a new one. If not possible, game over.
      if (!tileOccupied(game.activeTile) || !moveDown()) {
        if (addNewTile()) {
          game.state |= TILE_ADDED;
          game.tiles++;
        } else {
          gameOver();
        }
      }
    }
  }

  // Press any key to start a new game
  if ((game.state == GAMEOVER) && key) {
    playfieldChanged = true;
    newGame();
    addNewTile();
    game.state |= TILE_ADDED;
    game.tiles++;
  }

  return playfieldChanged;
}

int readKeyboard() {
  // Because of screen thing:
  return 0;
  struct pollfd pollStdin = {
       .fd = STDIN_FILENO,
       .events = POLLIN
  };
  int lkey = 0;

  if (poll(&pollStdin, 1, 0)) {
    lkey = fgetc(stdin);
    if (lkey != 27)
      goto exit;
    lkey = fgetc(stdin);
    if (lkey != 91)
      goto exit;
    lkey = fgetc(stdin);
  }
 exit:
    switch (lkey) {
      case 10: return KEY_ENTER;
      case 65: return KEY_UP;
      case 66: return KEY_DOWN;
      case 67: return KEY_RIGHT;
      case 68: return KEY_LEFT;
    }
  return 0;
}

void renderConsole(bool const playfieldChanged) {
  if (!playfieldChanged)
    return;

  // Goto beginning of console
  fprintf(stdout, "\033[%d;%dH", 0, 0);
  for (unsigned int x = 0; x < game.grid.x + 2; x ++) {
    fprintf(stdout, "-");
  }
  fprintf(stdout, "\n");
  for (unsigned int y = 0; y < game.grid.y; y++) {
    fprintf(stdout, "|");
    for (unsigned int x = 0; x < game.grid.x; x++) {
      coord const checkTile = {x, y};
      fprintf(stdout, "%c", (tileOccupied(checkTile)) ? '#' : ' ');
    }
    switch (y) {
      case 0:
        fprintf(stdout, "| Tiles: %10u\n", game.tiles);
        break;
      case 1:
        fprintf(stdout, "| Rows:  %10u\n", game.rows);
        break;
      case 2:
        fprintf(stdout, "| Score: %10u\n", game.score);
        break;
      case 4:
        fprintf(stdout, "| Level: %10u\n", game.level);
        break;
      case 7:
        fprintf(stdout, "| %17s\n", (game.state == GAMEOVER) ? "Game Over" : "");
        break;
    default:
        fprintf(stdout, "|\n");
    }
  }
  for (unsigned int x = 0; x < game.grid.x + 2; x++) {
    fprintf(stdout, "-");
  }
  fflush(stdout);
}


inline unsigned long uSecFromTimespec(struct timespec const ts) {
  return ((ts.tv_sec * 1000000) + (ts.tv_nsec / 1000));
}

int main(int argc, char **argv) {
  (void) argc;
  (void) argv;
  // This sets the stdin in a special state where each
  // keyboard press is directly flushed to the stdin and additionally
  // not outputted to the stdout
  {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag &= ~(ICANON | ECHO);
    ttystate.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
  }

  // Allocate the playing field structure
  game.rawPlayfield = (tile *) malloc(game.grid.x * game.grid.y * sizeof(tile));
  game.playfield = (tile**) malloc(game.grid.y * sizeof(tile *));
  if (!game.playfield || !game.rawPlayfield) {
    fprintf(stderr, "ERROR: could not allocate playfield\n");
    return 1;
  }
  for (unsigned int y = 0; y < game.grid.y; y++) {
    game.playfield[y] = &(game.rawPlayfield[y * game.grid.x]);
  }

  // Reset playfield to make it empty
  resetPlayfield();
  // Start with gameOver
  gameOver();

  if (!initializeSenseHat()) {
    fprintf(stderr, "ERROR: could not initilize sense hat\n");
    return 1;
  };

  // Clear console, render first time
  fprintf(stdout, "\033[H\033[J");
  renderConsole(true);
  renderSenseHatMatrix(true);

  while (true) {
    struct timeval sTv, eTv;
    gettimeofday(&sTv, NULL);

    int key = readSenseHatJoystick();
    if (!key)
      key = readKeyboard();
    if (key == KEY_ENTER)
      break;

    bool playfieldChanged = sTetris(key);
    renderConsole(playfieldChanged);
    renderSenseHatMatrix(playfieldChanged);

    // Wait for next tick
    gettimeofday(&eTv, NULL);
    unsigned long const uSecProcessTime = ((eTv.tv_sec * 1000000) + eTv.tv_usec) - ((sTv.tv_sec * 1000000 + sTv.tv_usec));
    if (uSecProcessTime < game.uSecTickTime) {
      usleep(game.uSecTickTime - uSecProcessTime);
    }
    game.tick = (game.tick + 1) % game.nextGameTick;
  }

  freeSenseHat();
  free(game.playfield);
  free(game.rawPlayfield);

  return 0;
}