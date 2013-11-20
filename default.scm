(use-modules (ice-9 match))

(it-switch-mode)

(define input "")

(define (apply-input-to func)
  (set! input "")
  (set! waiter
        (lambda (key ctrl mod1)
          (let ((char (integer->char key)))
            (if (eqv? char #\return)
                (begin
                  (set! waiter default-waiter)
                  (func input))
                (set! input (string-append input (string char))))))))

(define (apply-numeric-input-to func)
  (apply-input-to (lambda (numstr)
                    (func (if (string->number numstr)
                              (string->number numstr)
                              0)))))

(define (default-waiter key ctrl mod1)
  (if ctrl
      (match (integer->char key)
        (#\6 (i-alternate))
        (#\n (i-navigate-frame 1))
        (#\p (i-navigate-frame -1))
        (#\space (i-toggle-animation))
        (else (display (list 'unknown-command key ctrl mod1))))
      (match (integer->char key)
        (#\q (it-quit))
        (#\return (it-switch-mode))
        (#\f (it-toggle-fullscreen))
        (#\b (it-toggle-bar))
        (#\r (it-reload-image))
        (#\R (t-reload-all))
        (#\D (it-remove-image))
        (#\n (i-navigate 1))
        (#\space (i-navigate 1))
        (#\p (i-navigate -1))
        (#\backspace (i-navigate -1))
        (#\g (it-first))
        (#\G (apply-numeric-input-to it-n-or-last))
        (else (display (list 'unknown-command key ctrl mod1))))))

(define waiter default-waiter)

(define (on-key-press key ctrl mod1)
  (waiter key ctrl mod1)
  #t)
