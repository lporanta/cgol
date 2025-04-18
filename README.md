# Conway's Game of Life in C

<video src="https://github.com/user-attachments/assets/9cbd9691-b241-43e6-8bd3-04a1ab837876" width=50/></video>

## Compiling and installing

### Dependencies

* ncurses
* argparse

### Installation

Clone the repo, cd into the direcory and run make

```bash
git clone https://github.com/lporanta/cgol
cd cgol
make
chmod +x cgol
```

Now you can try it out and possibly move the 'cgol' binary somewhere in your $PATH

```bash
# for example
sudo mv cgol /usr/local/bin/ 
```

## Usage

### Arguments

> -c, --color_list=0,1,...,255   List of colors (D=0,...,7)\
> -f, --fps=[0-]             Frames per second, 0 means no delay (D=30)\
> -i, --init_ticks=[0-]      Ticks to advance before first print (D=5)\
> -l, --char_alive=[CHAR]    Char for the cells (D=█)\
> -p, --init_prob=[0-100]    Cell initialization probability in % (D=12)\
> -r, --ticks_limit=[0-]     Ticks before reset, 0 means no reset (D=240)\
> -?, --help                 Give this help list

### Colors

> black   0\
> red     1\
> green   2\
> yellow  3\
> blue    4\
> magenta 5\
> cyan    6\
> white   7\
> ...

### Controls

> c: clear and reset the screen\
> f: change color\
> F: change color in reverse order\
> esc|q|enter: quit
