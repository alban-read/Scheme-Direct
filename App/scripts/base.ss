 
(debug-on-exception #f)

(define gc
  (lambda () (collect) (collect) (collect) (collect)))
 
(define escape-pressed?
  (lambda () ((foreign-procedure "EscapeKeyPressed" () ptr))))

(define-syntax dotimes
  (syntax-rules ()
    [(_ n body ...)
     (let loop ([i n])
       (when (< 0 i)
         body
         ...
         (loop (- i 1))))]))

(define-syntax while
  (syntax-rules ()
    ((while condition body ...)
     (let loop ()
       (if condition
           (begin
             body ...
             (loop))
           #f)))))

(define-syntax for
  (syntax-rules (for in to step)
    [(for i in elements body ...)
     (for-each (lambda (i) body ...) elements)]
    [(for i from start to end step is body ...)
     (let ([condition (lambda (i)
                        (cond
                          [(< is 0) (< i end)]
                          [(> is 0) (> i end)]
                          [else #f]))])
       (do ([i start (+ i is)]) ((condition i) (void)) body ...))]
    [(for i from start to end body ...)
     (do ([i start (+ i 1)]) ((> i end) (void)) body ...)]))

(define-syntax try
  (syntax-rules (catch)
    [(_ body (catch catcher))
     (call-with-current-continuation
       (lambda (exit)
         (with-exception-handler
           (lambda (condition) (catcher condition) (exit condition))
           (lambda () body))))]))

;; timers 

(define set-repaint-timer
  (lambda (n)
    ((foreign-procedure "set_repaint_timer" (int) ptr) n)))

(define set-every-function
  (lambda (d p m f)
    ((foreign-procedure "every" (int int int ptr) ptr) d p m f)))
		
(define set-every-timer
 (lambda (d p m)
	(set-every-function d p m  every_step )))
	
;; run p after n 
(define after 
	(lambda (d n) 
		  ((foreign-procedure "after" (int ptr) ptr) d n)))

 
;; direct 2d  

(define identity
 (lambda ()
   ((foreign-procedure "d2d_matrix_identity"
    () ptr))))
 
(define rotate
 (lambda (a x y)
   ((foreign-procedure "d2d_matrix_rotate"
    (float float string) ptr) a x y)))

(define show
 (lambda ( n)
   ((foreign-procedure "d2d_show"
    (int) ptr) n)))

(define render
 (lambda (x y)
   ((foreign-procedure "d2d_render"
    (float float) ptr) x y)))

(define font
 (lambda (face size)
   ((foreign-procedure "d2d_set_font"
    (string float) ptr) face size)))

(define write-text
 (lambda (x y s)
   ((foreign-procedure "d2d_write_text"
    (float float string) ptr) x y s)))

(define draw-rect
 (lambda (x y w h)
   ((foreign-procedure "d2d_rectangle"
    (float float float float) ptr) x y w h)))

(define draw-ellipse
 (lambda (x y w h)
   ((foreign-procedure "d2d_ellipse"
    (float float float float) ptr) x y w h)))

(define fill-ellipse
 (lambda (x y w h)
   ((foreign-procedure "d2d_fill_ellipse"
    (float float float float) ptr) x y w h)))

(define fill-rect
 (lambda (x y w h)
   ((foreign-procedure "d2d_fill_rectangle"
    (float float float float) ptr) x y w h)))

 (define fill-colour
 (lambda (r g b a)
   ((foreign-procedure "d2d_fill_color"
    (float float float float) ptr) r g b a)))
	
(define line-colour
 (lambda (r g b a)
   ((foreign-procedure "d2d_color"
    (float float float float) ptr) r g b a)))
	
 
(define load-sprites
 (lambda (s n)
   ((foreign-procedure "d2d_load_sprites"
    (string int) ptr) s n)))

 
(define draw-sprite
 (lambda (n dx dy)
   ((foreign-procedure "d2d_render_sprite"
    (int float float  ) 
		ptr) n dx dy)))	
		
(define draw-scaled-rotated-sprite
 (lambda (n dx dy s a)
   ((foreign-procedure "d2d_render_sprite_rotscale"
    (int float float float float  ) 
		ptr) n dx dy s a)))	
		
		
(define draw-tiled-sprite
 (lambda (n dx dy s a)
   ((foreign-procedure "d2d_brush_tiled_sprite_rotscale"
    (int float float float float  ) 
		ptr) n dx dy s a)))	
		
	
(define render-sprite
 (lambda (n dx dy dh dw
            sx sy sh sw scale)
   ((foreign-procedure "d2d_render_sprite_sheet"
    (int float float float float 
		 float float float float
		 float) 
		ptr) n dx dy dh dw
			sx sy sh sw scale)))		
			
;; used to track keys in graphics window
(define graphics-keys
 (lambda ()
   ((foreign-procedure "graphics_keys"
    () ptr))))
	
;; sound 
(define load-sound
 (lambda (s n)
   ((foreign-procedure "load_sound"
    (string int) ptr) s n)))

(define play-sound
  (lambda (n)
   ((foreign-procedure "play_sound"
    (int) ptr) n)))

 
 
;; example just draws circles.
 
(define circlecount 1000)

;; make a new circle
(define newcircle
  (lambda ()
    (list
      (list (random 800.0) (random 600.0))
      (list (- 5.0 (random 10.0)) (- 5.0 (random 10.0)))
      (list (random 1.0) (random 1.0) (random 1.0)))))

;; make n new circles
(define newcircles 
  (lambda (n) 
	(let ([l '()])
	  (dotimes n 
		(set! l (append l (list (newcircle))))) l)))

;; keep a list of circles
(define circles 
  (newcircles circlecount))

;; unjam any circle that is stuck	
(define unstickv 
 (lambda (v) 
	(list (if (= (car v) 0) (- 5.0 (random 10.0)) (car v))
		  (if (= (cadr v) 0) (- 5.0 (random 10.0)) (cadr v))))) 

(define count-offscreen
 (lambda ()
	(let ([count 0])
	 (for e in circles 
	  (when 
		(or 
		 (> (caar e) 800.0) 
		 (< (caar e) 0.0)
		 (> (cadar e) 600.0) 
		 (< (cadar e) 0.0))
			(set! count (+ count 1))))  count ))) 
			
(define few-circles
	(lambda ()
	 (>= (count-offscreen) (- circlecount 20))))



;; move all circles
(define move-circles
 (lambda (c)
 (list (map + (car c)(cadr c)) (unstickv (cadr c)) (caddr c))))

;; draw a circle
(define drawcirc
 (lambda (c) 
	(apply fill-colour (append (caddr c) (list 0.5)))
	(apply line-colour (append (caddr c) (list 1.0)))
    (apply fill-ellipse (append (car c) (list 50.0 50.0)))
	(apply draw-ellipse (append (car c) (list 50.0 50.0)))))


(define show-status
 (lambda ()
	(fill-colour 1.0 0.5 0.9 1.0)
	(font "Calibri" 32.0)
	(write-text 5.0 5.0 
		(string-append "Circles:"
		 (number->string
			(- circlecount (count-offscreen)))))))	
			 

;; perform one step
(define circle-step
 (lambda ()
	(fill-colour 0.0 0.0 0.0 1.0)
	(fill-rect 0.0 0.0 800.0 600.0)
	(map drawcirc circles)
 	( show-status)
	(when (few-circles) 
		(set! circles 
			(newcircles circlecount)))
	(set! circles (map move-circles circles))))
 
 
(set-repaint-timer 33)

;; run circle step on the repeating timer.
(set-every-function 1000 60 0 
		(lambda ()
		  (circle-step)(gc)  ))
 
 
 
	  