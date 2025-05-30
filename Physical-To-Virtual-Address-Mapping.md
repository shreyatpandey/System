# Paging and Memory Management Concepts

## Page
One of numerous equally sized chunks of memory.

## Page Table
Stores where in memory each page is.

## Main Memory
Divided into page frames, a space large enough to hold one page of data (e.g., 4k).

## Swap Space
- Divided into pages
- Assume the complete process is first loaded into the swap space
- More pages go back and forth between swap space and main memory
- When a program is loaded, put it into the swap space
- Memory and page size is always a power of 2

---

## Address Translation with Paging
For each process there is an address `A`.

### 1. Page Number Calculation
- `page number = A / page_size`
- This is the page number within the process address space
- Example: Address in the process, `A = 10,000`, page size = 4k
  - `page number = 10000 / 4096 = 2.xxx` (truncate to 2)
- Since the page size is a power of 2 (e.g., 4k = 2^12), to determine the page number, shift the address right by 12 bits
- If the virtual address size = 32 bits, then:
  - The last 12 bits give the page offset
  - The first 32 - 12 = 20 bits give the page number
- **Example Address (in binary):**
  - | 10101010101010101010 | 101010101010 |

### 2. Offset Calculation
- `offset = A mod page_size`
- This is the distance from the beginning of the page
- Example: Address in the process, `A = 10,000`, page size = 4k
  - `page offset = 10000 mod 4096 = 1908`
- To determine the page offset, mask out all but the rightmost 12 bits

### Physical Address Calculation
1. Look up the page number in the page table and obtain the frame number
2. To create the physical address:
   - Frame = 17 bits; Offset = 12 bits
   - If main memory is 512k, then the physical address is 29 bits

---

## Effective Memory Access Time
- The percentage of times that a page number is found in the associative registers is called the **hit ratio**
- Example: 80% hit ratio
  - 20 ns to search associative registers
  - 100 ns to access memory
  - Mapped memory access: 120 ns when the page number is in the associative registers
  - If not found: 20 ns (search) + 100 ns (page table) + 100 ns (memory) = 220 ns
- **Effective access time:**
  - `0.80 * 120 + 0.20 * 220 = 140 ns`
  - For 98% hit ratio: `0.98 * 120 + 0.02 * 220 = 122 ns`
- The hit ratio is related to the number of associative registers (16 to 512 yields 80-98% hit ratio)

### Translation Lookaside Buffer (TLB)
- Used to do page table look-ups quickly
- Associative cache (fastest memory, all entries checked at once)
- Expensive, relatively small
- Stores entries from the most recently used pages
- Updated each time a page fault occurs

---

## Page Fault
- When a process tries to access an address whose page is not currently in memory
- Process must be suspended, leaves the processor and the ready list, status is now "waiting for main memory"
- Address can be of an instruction or data (heap, stack, static, variables)

---

## Paging Operations
### Fetch "page in" (bring a page into main memory)

#### 1. Fetch Policy
- **a) Demand Fetching (Demand Paging):**
  - Fetch a page when it is referenced but not stored in main memory
  - Causes a page fault whenever a new page is required
  - Disadvantage: cold start fault (many page faults when a process is just starting)
  - Advantage: no unnecessary pages are ever fetched
- **b) Anticipatory Fetching (Prepaging):**
  - Guess which pages will be required soon and fetch them before they are referenced

##### Variations:
- **i) Working Set Prepaging:**
  - Fetch all pages in a process' working set before restarting
  - Working set: set of pages accessed in the last 'w' working time units (window size based on temporal locality)
  - Intent: all pages required soon are in main memory when the process starts
- **ii) Clustering Prepaging:**
  - When a page is fetched, also fetch the next page(s) in the process address space
  - Cost of fetching n consecutive pages from disk is less than fetching n non-consecutive pages
  - Common variant: fetch pairs of pages whenever one is referenced (used in Windows NT, Windows 2000)
- **iii) Advised Prepaging:**
  - Programmer/compiler adds hints to the OS about what pages will be needed soon (e.g., hint: `&myfunction`)
  - Problem: cannot trust programmers; they may hint that all their pages are important

#### 2. Placement Policy
- Determine where to put the page that has been fetched
- Easy for paging: just use any free page frame

#### 3. Replacement Policy
- Determines which page should be removed from main memory (when a page must be fetched)
- Want to find the least useful page in main memory
- Candidates, in order of preference:
  1. Page of a terminated process
  2. Page of a long blocked process
     - Danger: do not want to swap out pages from a process that is trying to bring pages into main memory
  3. Take a page from a ready process that has not been referenced for a long time
  4. Take a page that has not been modified since it was swapped in (saves copying to the swap space)
  5. Take a page that has been referenced recently by a ready process (performance will downgrade)

- **Thrashing:**
  - System is preoccupied with moving pages in and out of memory
  - Disk can be very busy while the CPU is nearly idle
  - One cure: reduce the number of processes in main memory (reduce the level of multiprogramming)

---

## Local vs Global Page Replacement
- **Local page replacement:** when a process pages "against itself" and removes some of its own pages
- **Global page replacement:** when pages from all processes are considered
- Example: Windows NT/XP/Vista use both a local page replacement method (based on FIFO) and a global page replacement method (based on PFF)
