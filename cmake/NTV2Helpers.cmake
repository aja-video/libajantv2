function(aja_ntv2_sdk_gen target_deps)
	add_custom_command(
		TARGET ${target_deps} PRE_BUILD
		COMMAND
			${Python2_EXECUTABLE} ${AJA_NTV2_ROOT}/ajalibraries/ajantv2/sdkgen/ntv2sdkgen.py
			--verbose --unused --ajantv2 ajalibraries/ajantv2 --ohh ajalibraries/ajantv2/includes --ohpp ajalibraries/ajantv2/src
		WORKING_DIRECTORY ${AJA_NTV2_ROOT}
		COMMENT "Running ntv2sdkgen script...")
endfunction(aja_ntv2_sdk_gen)
