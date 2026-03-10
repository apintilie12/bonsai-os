.globl spin_lock
spin_lock:
	mov	w1, #1
1:
	ldaxr	w2, [x0]	// Load with acquire semantics
	cbnz	w2, 1b		// If lock is already held (val != 0), spin
	stxr	w2, w1, [x0]	// Attempt to set lock to 1
	cbnz	w2, 1b		// If stxr failed (w2 != 0), retry
	ret

.globl spin_unlock
spin_unlock:
	stlr	wzr, [x0]	// Store 0 with release semantics
	ret