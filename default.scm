(use-modules (ice-9 match))

(define constants
  '((left 0)
    (right 1)
    (up 2)
    (down 3)
    (scale-down 0)
    (scale-fit 1)
    (scale-width 2)
    (scale-height 3)
    (scale-zoom 4)
    (degree-90 1)
    (degree-180 2)
    (degree-270 3)
    (flip-horizontal 0)
    (flip-vertical 1)
    (mouse-left 1)
    (mouse-middle 2)
    (mouse-right 3)
    (mouse-up 4)
    (mouse-down 5)))

(define-syntax const
  (syntax-rules ()
    ((_ name)
     (cadr (assoc 'name constants)))))

(it-switch-mode)

(define *input* "")

(define (apply-input-to func)
  (display "waiting for a number ")
  (display func) (newline)
  (set! *input* "")
  (set! waiter
        (lambda (key ctrl mod1)
          (let ((char (integer->char key)))
            (if (eqv? char #\return)
                (begin
                  (set! waiter default-waiter)
                  (func *input*))
                (set! *input* (string-append *input* (string char))))))))

(define (apply-numeric-input-to func)
  (apply-input-to (lambda (numstr)
                    (func (if (string->number numstr)
                              (string->number numstr)
                              0)))))

(define (default-waiter key ctrl mod1)
  ;(newline)
  (display (list 'command key ctrl mod1))
  (newline)
  (if (> key 0)
      (if ctrl
          (match (integer->char key)
            (#\6 (i-alternate))
            (#\n (i-navigate-frame 1))
            (#\p (i-navigate-frame -1))
            (#\space (i-toggle-animation))
            (#\h (it-scroll-screen (const left)))
            (#\j (it-scroll-screen (const down)))
            (#\k (it-scroll-screen (const up)))
            (#\l (it-scroll-screen (const right)))
            (else #f))
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
            (#\h (apply-numeric-input-to (lambda (num) (it-scroll-move (const left) num))))
            (#\j (apply-numeric-input-to (lambda (num) (it-scroll-move (const down) num))))
            (#\k (apply-numeric-input-to (lambda (num) (it-scroll-move (const up) num))))
            (#\l (apply-numeric-input-to (lambda (num) (it-scroll-move (const right) num))))
            (#\H (i-scroll-to-edge (const left)))
            (#\J (i-scroll-to-edge (const down)))
            (#\K (i-scroll-to-edge (const up)))
            (#\L (i-scroll-to-edge (const right)))
            (#\+ (i-zoom 1))
            (#\- (i-zoom -1))
            (#\= (i-set-zoom 100))
            (#\w (i-fit-to-win (const scale-fit)))
            (#\e (i-fit-to-win (const scale-width)))
            (#\E (i-fit-to-win (const scale-height)))
            (#\W (i-fit-to-img))
            (#\< (i-rotate (const degree-270)))
            (#\> (i-rotate (const degree-90)))
            (#\? (i-rotate (const degree-180)))
            (#\| (i-flip (const flip-horizontal)))
            (#\_ (i-flip (const flip-vertical)))
            (#\a (i-toggle-antialias))
            (#\A (it-toggle-alpha))
            (else #f)
            ))
      #f))

(define waiter default-waiter)

(define (on-key-press key ctrl mod1)
  (waiter key ctrl mod1)
  #t)

(define (on-button-press button ctrl x y)
  (display (list button ctrl x y))
  (match button
    (4 (if ctrl
           (it-scroll-move (const left) 42)
           (it-scroll-move (const up) 42)))
    (5 (if ctrl
           (it-scroll-move (const right) 42)
           (it-scroll-move (const down) 42)))
    (2 (it-toggle-fullscreen))
    (1 (i-navigate 1))
    (3 (i-navigate -1))
    (else #f)))
