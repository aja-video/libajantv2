function Resequence() range
	let lnum = a:firstline
	let testnum = 1
	while lnum <= a:lastline
		let line = getline(lnum)
		if line =~ '\~'
			let repl = substitute(line, '\~', testnum, "g")
			call setline(lnum, repl)
			let testnum = testnum + 1
		endif
		let lnum = lnum + 1
	endwhile
endfunction

