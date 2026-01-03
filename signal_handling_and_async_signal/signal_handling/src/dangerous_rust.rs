use std::vec::Vec;

// DANGER: This function is NOT async-signal-safe!
// It allocates memory, which can deadlock if called from signal handler
#[no_mangle]
pub extern "C" fn rust_cleanup() {
    // This allocation can deadlock if:
    // 1. Main thread holds allocator lock
    // 2. Signal interrupts main thread
    // 3. This handler tries to acquire same lock
    
    let mut vec = Vec::with_capacity(1024);
    for i in 0..100 {
        vec.push(i);  // Allocates memory - NOT safe in signal handler!
    }
    
    // The drop here also touches the allocator
    drop(vec);
}
