use std::vec::Vec;

// SAFE: This is called from normal context, not signal handler
#[no_mangle]
pub extern "C" fn rust_process_signal(signal_num: i32) {
    // Now we can do ANYTHING - we're in normal execution context
    // The signal was blocked and converted to an fd event
    
    println!("  [Rust] Processing signal {} safely", signal_num);
    
    // Safe to allocate
    let mut data = Vec::with_capacity(1000);
    for i in 0..1000 {
        data.push(i * signal_num);
    }
    
    // Safe to use String
    let message = format!("Processed {} items for signal {}", data.len(), signal_num);
    println!("  [Rust] {}", message);
    
    // Safe to drop - no risk of deadlock
    drop(data);
    
    println!("  [Rust] Cleanup completed safely");
}
