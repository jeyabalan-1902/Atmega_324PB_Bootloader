/*
 * Bootloader_ATMEGA 324PB.c
 * Created: 03-07-2024 16:52:56
 * Author : JEYABALAN K
 */


#include <avr/io.h>
#include <avr/boot.h>
#include <avr/interrupt.h>

#define PAGESIZEB 128 // Define the page size in bytes for ATmega 324PB

void do_spm(uint8_t spmcsrval);
void write_page(uint16_t ram_addr, uint32_t flash_addr);
void check_rww_section(void);

void write_page(uint16_t ram_addr, uint32_t flash_addr) {
    uint16_t loop;

    // Page Erase
    do_spm((1 << PGERS) | (1 << SPMEN));

    // Re-enable the RWW section
    do_spm((1 << RWWSRE) | (1 << SPMEN));

    // Transfer data from RAM to Flash page buffer
    for (loop = 0; loop < PAGESIZEB; loop += 2) {
        uint16_t data = *((uint16_t *)(ram_addr + loop));
        boot_page_fill(flash_addr + loop, data);
    }

    // Execute Page Write
    do_spm((1 << PGWRT) | (1 << SPMEN));

    // Re-enable the RWW section
    do_spm((1 << RWWSRE) | (1 << SPMEN));
}

void do_spm(uint8_t spmcsrval) {
    uint8_t temp;

    // Check for previous SPM complete
    while (SPMCSR & (1 << SPMEN));

    // Disable interrupts if enabled, store status
    temp = SREG;
    cli();

    // Check that no EEPROM write access is present
    while (EECR & (1 << EEPE));

    // SPM timed sequence
    SPMCSR = spmcsrval;
    __asm__ __volatile__ ("spm");

    // Restore SREG (to enable interrupts if originally enabled)
    SREG = temp;
}

void check_rww_section(void) {
    uint8_t temp1;

    do {
        temp1 = SPMCSR;
    } while (temp1 & (1 << RWWSB));

    do_spm((1 << RWWSRE) | (1 << SPMEN));
}

int main(void) {
    uint16_t ram_addr = 0x0100;  // Example RAM address
    uint32_t flash_addr = 0x0000; // Example Flash address

    // Disable interrupts if not already disabled
    cli();

    // Write page
    write_page(ram_addr, flash_addr);

    // Check RWW section and return to application code
    check_rww_section();

    // Jump to the application start address (reset vector)
    asm volatile ("jmp 0x0000");

    // Application code should not be here
    while (1);
}
