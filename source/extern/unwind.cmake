# other source files

set(UNWIND_PATH "libunwind")


#if(TARGET_ARM)
#    # Ensure that the remote and local unwind code can reside in the same binary without name clashing
#    add_definitions("-Darm_search_unwind_table=UNW_OBJ(arm_search_unwind_table)")
#    # We compile code with -std=c99 and the asm keyword is not recognized as it is a gnu extension
#    add_definitions(-Dasm=__asm__)
#    # The arm sources include ex_tables.h from include/tdep-arm without going through a redirection
#    # in include/tdep like it works for similar files on other architectures. So we need to add
#    # the include/tdep-arm to include directories
#    include_directories(../include/tdep-arm)
#elseif(TARGET_AARCH64)
#    # We compile code with -std=c99 and the asm keyword is not recognized as it is a gnu extension
#    add_definitions(-Dasm=__asm__)
#endif()

SET(libunwind_ptrace_la_SOURCES
    ${UNWIND_PATH}/src/mi/init.c
    ${UNWIND_PATH}/src/ptrace/_UPT_elf.c
    ${UNWIND_PATH}/src/ptrace/_UPT_accessors.c ptrace/_UPT_access_fpreg.c
    ${UNWIND_PATH}/src/ptrace/_UPT_access_mem.c ptrace/_UPT_access_reg.c
    ${UNWIND_PATH}/src/ptrace/_UPT_create.c ptrace/_UPT_destroy.c
    ${UNWIND_PATH}/src/ptrace/_UPT_find_proc_info.c ptrace/_UPT_get_dyn_info_list_addr.c
    ${UNWIND_PATH}/src/ptrace/_UPT_put_unwind_info.c ptrace/_UPT_get_proc_name.c
    ${UNWIND_PATH}/src/ptrace/_UPT_reg_offset.c ptrace/_UPT_resume.c
    ${UNWIND_PATH}/src/ptrace/_UPT_get_elf_filename.c ptrace/_UPT_ptrauth_insn_mask.c
)

SET(libunwind_coredump_la_SOURCES
    ${UNWIND_PATH}/src/coredump/_UCD_accessors.c
    ${UNWIND_PATH}/src/coredump/_UCD_create.c
    ${UNWIND_PATH}/src/coredump/_UCD_destroy.c
    ${UNWIND_PATH}/src/coredump/_UCD_access_mem.c
    ${UNWIND_PATH}/src/coredump/_UCD_elf_map_image.c
    ${UNWIND_PATH}/src/coredump/_UCD_find_proc_info.c
    ${UNWIND_PATH}/src/coredump/_UCD_get_proc_name.c
    ${UNWIND_PATH}/src/coredump/_UCD_get_elf_filename.c

    ${UNWIND_PATH}/src/mi/init.c
    ${UNWIND_PATH}/src/coredump/_UPT_elf.c
    ${UNWIND_PATH}/src/coredump/_UPT_access_fpreg.c
    ${UNWIND_PATH}/src/coredump/_UPT_get_dyn_info_list_addr.c
    ${UNWIND_PATH}/src/coredump/_UPT_put_unwind_info.c
    ${UNWIND_PATH}/src/coredump/_UPT_resume.c
)

# List of arch-independent files needed by generic library (libunwind-$ARCH):
SET(libunwind_la_SOURCES_generic
    ${UNWIND_PATH}/src/mi/Gdyn-extract.c 
    ${UNWIND_PATH}/src/mi/Gdyn-remote.c 
    ${UNWIND_PATH}/src/mi/Gfind_dynamic_proc_info.c
    # The Gget_accessors.c implements the same function as Lget_accessors.c, so
    # the source is excluded here to prevent name clash
    #mi/Gget_accessors.c
    ${UNWIND_PATH}/src/mi/Gget_proc_info_by_ip.c 
    ${UNWIND_PATH}/src/mi/Gget_proc_name.c
    ${UNWIND_PATH}/src/mi/Gput_dynamic_unwind_info.c 
    ${UNWIND_PATH}/src/mi/Gdestroy_addr_space.c
    ${UNWIND_PATH}/src/mi/Gget_reg.c ${UNWIND_PATH}/src/mi/Gset_reg.c
    ${UNWIND_PATH}/src/mi/Gget_fpreg.c ${UNWIND_PATH}/src/mi/Gset_fpreg.c
    ${UNWIND_PATH}/src/mi/Gset_caching_policy.c
    ${UNWIND_PATH}/src/mi/Gset_cache_size.c
    ${UNWIND_PATH}/src/mi/Gset_iterate_phdr_function.c
    ${UNWIND_PATH}/src/mi/Gget_elf_filename.c
)

SET(libunwind_la_SOURCES_os_linux
    ${UNWIND_PATH}/src/os-linux.c
)

SET(libunwind_la_SOURCES_os_linux_local
# Nothing when we don't want to support CXX exceptions
)

SET(libunwind_la_SOURCES_os_freebsd
    ${UNWIND_PATH}/src/os-freebsd.c
)

SET(libunwind_la_SOURCES_os_freebsd_local
# Nothing
)

SET(libunwind_la_SOURCES_os_solaris
    ${UNWIND_PATH}/src/os-solaris.c
)

SET(libunwind_la_SOURCES_os_solaris_local
# Nothing
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(libunwind_la_SOURCES_os                 ${libunwind_la_SOURCES_os_linux})
    SET(libunwind_la_SOURCES_os_local           ${libunwind_la_SOURCES_os_linux_local})
    SET(libunwind_la_SOURCES_x86_os             ${UNWIND_PATH}/src/x86/Gos-linux.c)
    SET(libunwind_x86_la_SOURCES_os             ${UNWIND_PATH}/src/x86/getcontext-linux.S)
    SET(libunwind_la_SOURCES_x86_os_local       ${UNWIND_PATH}/src/x86/Los-linux.c)
    SET(libunwind_la_SOURCES_x86_64_os          ${UNWIND_PATH}/src/x86_64/Gos-linux.c)
    SET(libunwind_la_SOURCES_x86_64_os_local    ${UNWIND_PATH}/src/x86_64/Los-linux.c)
    SET(libunwind_la_SOURCES_arm_os             ${UNWIND_PATH}/src/arm/Gos-linux.c)
    SET(libunwind_la_SOURCES_arm_os_local       ${UNWIND_PATH}/src/arm/Los-linux.c)
    list(APPEND libunwind_coredump_la_SOURCES   ${UNWIND_PATH}/src/coredump/_UCD_access_reg_linux.c
                                                ${UNWIND_PATH}/src/coredump/_UCD_get_threadinfo_prstatus.c
                                                ${UNWIND_PATH}/src/coredump/_UCD_get_mapinfo_linux.c)
  #elseif(UNW_CMAKE_TARGET_FREEBSD)
  #    SET(libunwind_la_SOURCES_os                 ${libunwind_la_SOURCES_os_freebsd})
  #    SET(libunwind_la_SOURCES_os_local           ${libunwind_la_SOURCES_os_freebsd_local})
  #    SET(libunwind_la_SOURCES_x86_os             x86/Gos-freebsd.c)
  #    SET(libunwind_x86_la_SOURCES_os             x86/getcontext-freebsd.S)
  #    SET(libunwind_la_SOURCES_x86_os_local       x86/Los-freebsd.c)
  #    SET(libunwind_la_SOURCES_x86_64_os          x86_64/Gos-freebsd.c)
  #    SET(libunwind_la_SOURCES_x86_64_os_local    x86_64/Los-freebsd.c)
  #    SET(libunwind_la_SOURCES_arm_os             arm/Gos-freebsd.c)
  #    SET(libunwind_la_SOURCES_arm_os_local       arm/Los-freebsd.c)
  #    list(APPEND libunwind_coredump_la_SOURCES   coredump/_UCD_access_reg_freebsd.c
  #                                                coredump/_UCD_get_threadinfo_prstatus.c
  #                                                coredump/_UCD_get_mapinfo_generic.c)
  #elseif(UNW_CMAKE_HOST_SUNOS)
  #    SET(libunwind_la_SOURCES_os                 ${libunwind_la_SOURCES_os_solaris})
  #    SET(libunwind_la_SOURCES_os_local           ${libunwind_la_SOURCES_os_solaris_local})
  #    SET(libunwind_la_SOURCES_x86_64_os          x86_64/Gos-solaris.c)
  #    SET(libunwind_la_SOURCES_x86_64_os_local    x86_64/Los-solaris.c)
endif()

# List of arch-independent files needed by both local-only and generic
# libraries:
SET(libunwind_la_SOURCES_common
    ${libunwind_la_SOURCES_os}
    ${UNWIND_PATH}/src/mi/init.c 
    ${UNWIND_PATH}/src/mi/flush_cache.c 
    ${UNWIND_PATH}/src/mi/mempool.c 
    ${UNWIND_PATH}/src/mi/strerror.c
)

SET(libunwind_la_SOURCES_local_unwind
# Nothing when we don't want to support CXX exceptions
)

# List of arch-independent files needed by local-only library (libunwind):
SET(libunwind_la_SOURCES_local_nounwind
    ${libunwind_la_SOURCES_os_local}
    ${UNWIND_PATH}/src/mi/backtrace.c
    ${UNWIND_PATH}/src/mi/dyn-cancel.c 
    ${UNWIND_PATH}/src/mi/dyn-info-list.c 
    ${UNWIND_PATH}/src/mi/dyn-register.c
    ${UNWIND_PATH}/src/mi/Ldyn-extract.c 
    ${UNWIND_PATH}/src/mi/Lfind_dynamic_proc_info.c
    ${UNWIND_PATH}/src/mi/Lget_accessors.c
    ${UNWIND_PATH}/src/mi/Lget_proc_info_by_ip.c 
    ${UNWIND_PATH}/src/mi/Lget_proc_name.c
    ${UNWIND_PATH}/src/mi/Lput_dynamic_unwind_info.c 
    ${UNWIND_PATH}/src/mi/Ldestroy_addr_space.c
    ${UNWIND_PATH}/src/mi/Lget_reg.c   
    ${UNWIND_PATH}/src/mi/Lset_reg.c
    ${UNWIND_PATH}/src/mi/Lget_fpreg.c 
    ${UNWIND_PATH}/src/mi/Lset_fpreg.c
    ${UNWIND_PATH}/src/mi/Lset_caching_policy.c
    ${UNWIND_PATH}/src/mi/Lset_cache_size.c
    ${UNWIND_PATH}/src/mi/Lset_iterate_phdr_function.c
    ${UNWIND_PATH}/src/mi/Lget_elf_filename.c
)

SET(libunwind_la_SOURCES_local
    ${libunwind_la_SOURCES_local_nounwind}
    ${libunwind_la_SOURCES_local_unwind}
)

SET(libunwind_dwarf_common_la_SOURCES
    ${UNWIND_PATH}/src/dwarf/global.c
)

SET(libunwind_dwarf_local_la_SOURCES
    ${UNWIND_PATH}/src/dwarf/Lexpr.c 
    ${UNWIND_PATH}/src/dwarf/Lfde.c 
    ${UNWIND_PATH}/src/dwarf/Lparser.c 
    ${UNWIND_PATH}/src/dwarf/Lpe.c
    ${UNWIND_PATH}/src/dwarf/Lfind_proc_info-lsb.c
    ${UNWIND_PATH}/src/dwarf/Lfind_unwind_table.c
    ${UNWIND_PATH}/src/dwarf/Lget_proc_info_in_range.c
)

SET(libunwind_dwarf_generic_la_SOURCES
    ${UNWIND_PATH}/src/dwarf/Gexpr.c 
    ${UNWIND_PATH}/src/dwarf/Gfde.c 
    ${UNWIND_PATH}/src/dwarf/Gparser.c 
    ${UNWIND_PATH}/src/dwarf/Gpe.c
    ${UNWIND_PATH}/src/dwarf/Gfind_proc_info-lsb.c
    ${UNWIND_PATH}/src/dwarf/Gfind_unwind_table.c
    ${UNWIND_PATH}/src/dwarf/Gget_proc_info_in_range.c
)

SET(libunwind_elf32_la_SOURCES
    ${UNWIND_PATH}/src/elf32.c
)

SET(libunwind_elf64_la_SOURCES
    ${UNWIND_PATH}/src/elf64.c
)
SET(libunwind_elfxx_la_SOURCES
    ${UNWIND_PATH}/src/elfxx.c
)

# The list of files that go into libunwind and libunwind-aarch64:
SET(libunwind_la_SOURCES_aarch64_common
    ${libunwind_la_SOURCES_common}
    ${UNWIND_PATH}/src/aarch64/is_fpreg.c
    ${UNWIND_PATH}/src/aarch64/regname.c
)

# The list of files that go into libunwind:
SET(libunwind_la_SOURCES_aarch64
    ${libunwind_la_SOURCES_aarch64_common}
    ${libunwind_la_SOURCES_local}
    ${UNWIND_PATH}/src/aarch64/Lapply_reg_state.c ${UNWIND_PATH}/src/aarch64/Lreg_states_iterate.c
    ${UNWIND_PATH}/src/aarch64/Lcreate_addr_space.c ${UNWIND_PATH}/src/aarch64/Lget_proc_info.c
    ${UNWIND_PATH}/src/aarch64/Lget_save_loc.c ${UNWIND_PATH}/src/aarch64/Lglobal.c ${UNWIND_PATH}/src/aarch64/Linit.c
    ${UNWIND_PATH}/src/aarch64/Linit_local.c ${UNWIND_PATH}/src/aarch64/Linit_remote.c
    ${UNWIND_PATH}/src/aarch64/Lis_signal_frame.c ${UNWIND_PATH}/src/aarch64/Lregs.c ${UNWIND_PATH}/src/aarch64/Lresume.c
    ${UNWIND_PATH}/src/aarch64/Lstash_frame.c ${UNWIND_PATH}/src/aarch64/Lstep.c ${UNWIND_PATH}/src/aarch64/Ltrace.c
    ${UNWIND_PATH}/src/aarch64/Lstrip_ptrauth_insn_mask.c ${UNWIND_PATH}/src/aarch64/getcontext.S
)

SET(libunwind_aarch64_la_SOURCES_aarch64
    ${libunwind_la_SOURCES_aarch64_common}
    ${libunwind_la_SOURCES_generic}
    ${UNWIND_PATH}/src/aarch64/Gapply_reg_state.c 
    ${UNWIND_PATH}/src/aarch64/Greg_states_iterate.c
    ${UNWIND_PATH}/src/aarch64/Gcreate_addr_space.c 
    ${UNWIND_PATH}/src/aarch64/Gget_proc_info.c
    ${UNWIND_PATH}/src/aarch64/Gget_save_loc.c 
    ${UNWIND_PATH}/src/aarch64/Gglobal.c 
    ${UNWIND_PATH}/src/aarch64/Ginit.c
    ${UNWIND_PATH}/src/aarch64/Ginit_local.c 
    ${UNWIND_PATH}/src/aarch64/Ginit_remote.c
    ${UNWIND_PATH}/src/aarch64/Gis_signal_frame.c 
    ${UNWIND_PATH}/src/aarch64/Gregs.c 
    ${UNWIND_PATH}/src/aarch64/Gresume.c
    ${UNWIND_PATH}/src/aarch64/Gstash_frame.c 
    ${UNWIND_PATH}/src/aarch64/Gstep.c 
    ${UNWIND_PATH}/src/aarch64/Gtrace.c
    ${UNWIND_PATH}/src/aarch64/Gstrip_ptrauth_insn_mask.c
)

# The list of files that go into libunwind and libunwind-arm:
SET(libunwind_la_SOURCES_arm_common
    ${libunwind_la_SOURCES_common}
    ${UNWIND_PATH}/src/arm/is_fpreg.c 
    ${UNWIND_PATH}/src/arm/regname.c
)

# The list of files that go into libunwind:
SET(libunwind_la_SOURCES_arm
    ${libunwind_la_SOURCES_arm_common}
    ${libunwind_la_SOURCES_arm_os_local}
    ${libunwind_la_SOURCES_local}
    ${UNWIND_PATH}/src/arm/getcontext.S
    ${UNWIND_PATH}/src/arm/Lapply_reg_state.c 
    ${UNWIND_PATH}/src/arm/Lreg_states_iterate.c
    ${UNWIND_PATH}/src/arm/Lcreate_addr_space.c 
    ${UNWIND_PATH}/src/arm/Lget_proc_info.c 
    ${UNWIND_PATH}/src/arm/Lget_save_loc.c
    ${UNWIND_PATH}/src/arm/Lglobal.c 
    ${UNWIND_PATH}/src/arm/Linit.c 
    ${UNWIND_PATH}/src/arm/Linit_local.c 
    ${UNWIND_PATH}/src/arm/Linit_remote.c
    ${UNWIND_PATH}/src/arm/Lregs.c 
    ${UNWIND_PATH}/src/arm/Lresume.c 
    ${UNWIND_PATH}/src/arm/Lstep.c
    ${UNWIND_PATH}/src/arm/Lex_tables.c 
    ${UNWIND_PATH}/src/arm/Lstash_frame.c 
    ${UNWIND_PATH}/src/arm/Ltrace.c
)

# The list of files that go into libunwind-arm:
SET(libunwind_arm_la_SOURCES_arm
    ${libunwind_la_SOURCES_arm_common}
    ${libunwind_la_SOURCES_arm_os}
    ${libunwind_la_SOURCES_generic}
    ${UNWIND_PATH}/src/arm/Gapply_reg_state.c 
    ${UNWIND_PATH}/src/arm/Greg_states_iterate.c
    ${UNWIND_PATH}/src/arm/Gcreate_addr_space.c 
    ${UNWIND_PATH}/src/arm/Gget_proc_info.c 
    ${UNWIND_PATH}/src/arm/Gget_save_loc.c
    ${UNWIND_PATH}/src/arm/Gglobal.c 
    ${UNWIND_PATH}/src/arm/Ginit.c 
    ${UNWIND_PATH}/src/arm/Ginit_local.c 
    ${UNWIND_PATH}/src/arm/Ginit_remote.c
    ${UNWIND_PATH}/src/arm/Gregs.c 
    ${UNWIND_PATH}/src/arm/Gresume.c 
    ${UNWIND_PATH}/src/arm/Gstep.c
    ${UNWIND_PATH}/src/arm/Gex_tables.c 
    ${UNWIND_PATH}/src/arm/Gstash_frame.c 
    ${UNWIND_PATH}/src/arm/Gtrace.c
)

# The list of files that go both into libunwind and libunwind-x86_64:
SET(libunwind_la_SOURCES_x86_64_common
    ${libunwind_la_SOURCES_common}
    ${UNWIND_PATH}/src/x86_64/is_fpreg.c 
    ${UNWIND_PATH}/src/x86_64/regname.c
)

# The list of files that go into libunwind:
SET(libunwind_la_SOURCES_x86_64
    ${libunwind_la_SOURCES_x86_64_common}
    ${libunwind_la_SOURCES_x86_64_os_local}
    ${libunwind_la_SOURCES_local}
    ${UNWIND_PATH}/src/x86_64/setcontext.S
    ${UNWIND_PATH}/src/x86_64/Lapply_reg_state.c 
    ${UNWIND_PATH}/src/x86_64/Lreg_states_iterate.c
    ${UNWIND_PATH}/src/x86_64/Lcreate_addr_space.c 
    ${UNWIND_PATH}/src/x86_64/Lget_save_loc.c 
    ${UNWIND_PATH}/src/x86_64/Lglobal.c
    ${UNWIND_PATH}/src/x86_64/Linit.c 
    ${UNWIND_PATH}/src/x86_64/Linit_local.c 
    ${UNWIND_PATH}/src/x86_64/Linit_remote.c
    ${UNWIND_PATH}/src/x86_64/Lget_proc_info.c 
    ${UNWIND_PATH}/src/x86_64/Lregs.c 
    ${UNWIND_PATH}/src/x86_64/Lresume.c
    ${UNWIND_PATH}/src/x86_64/Lstash_frame.c 
    ${UNWIND_PATH}/src/x86_64/Lstep.c 
    ${UNWIND_PATH}/src/x86_64/Ltrace.c 
    ${UNWIND_PATH}/src/x86_64/getcontext.S
)

# The list of files that go into libunwind-x86_64:
SET(libunwind_x86_64_la_SOURCES_x86_64
    ${libunwind_la_SOURCES_x86_64_common}
    ${libunwind_la_SOURCES_x86_64_os}
    ${libunwind_la_SOURCES_generic}
    ${UNWIND_PATH}/src/x86_64/Gapply_reg_state.c ${UNWIND_PATH}/src/x86_64/Greg_states_iterate.c
    ${UNWIND_PATH}/src/x86_64/Gcreate_addr_space.c ${UNWIND_PATH}/src/x86_64/Gget_save_loc.c ${UNWIND_PATH}/src/x86_64/Gglobal.c
    ${UNWIND_PATH}/src/x86_64/Ginit.c ${UNWIND_PATH}/src/x86_64/Ginit_local.c ${UNWIND_PATH}/src/x86_64/Ginit_remote.c
    ${UNWIND_PATH}/src/x86_64/Gget_proc_info.c ${UNWIND_PATH}/src/x86_64/Gregs.c ${UNWIND_PATH}/src/x86_64/Gresume.c
    ${UNWIND_PATH}/src/x86_64/Gstash_frame.c ${UNWIND_PATH}/src/x86_64/Gstep.c ${UNWIND_PATH}/src/x86_64/Gtrace.c
)

# The list of files that go both into libunwind and libunwind-s390x:
SET(libunwind_la_SOURCES_s390x_common
    ${libunwind_la_SOURCES_common}
    ${UNWIND_PATH}/src/s390x/is_fpreg.c ${UNWIND_PATH}/src/s390x/regname.c
)

# The list of files that go into libunwind:
SET(libunwind_la_SOURCES_s390x
    ${libunwind_la_SOURCES_s390x_common}
    ${libunwind_la_SOURCES_local}
    ${UNWIND_PATH}/src/s390x/setcontext.S ${UNWIND_PATH}/src/s390x/getcontext.S
    ${UNWIND_PATH}/src/s390x/Lapply_reg_state.c ${UNWIND_PATH}/src/s390x/Lreg_states_iterate.c
    ${UNWIND_PATH}/src/s390x/Lcreate_addr_space.c ${UNWIND_PATH}/src/s390x/Lget_save_loc.c ${UNWIND_PATH}/src/s390x/Lglobal.c
    ${UNWIND_PATH}/src/s390x/Linit.c ${UNWIND_PATH}/src/s390x/Linit_local.c ${UNWIND_PATH}/src/s390x/Linit_remote.c
    ${UNWIND_PATH}/src/s390x/Lget_proc_info.c ${UNWIND_PATH}/src/s390x/Lregs.c ${UNWIND_PATH}/src/s390x/Lresume.c
    ${UNWIND_PATH}/src/s390x/Lis_signal_frame.c ${UNWIND_PATH}/src/s390x/Lstep.c
)

# The list of files that go into libunwind-s390x:
SET(libunwind_s390x_la_SOURCES_s390x
    ${libunwind_la_SOURCES_s390x_common}
    ${libunwind_la_SOURCES_generic}
    ${UNWIND_PATH}/src/s390x/Gapply_reg_state.c ${UNWIND_PATH}/src/s390x/Greg_states_iterate.c
    ${UNWIND_PATH}/src/s390x/Gcreate_addr_space.c ${UNWIND_PATH}/src/s390x/Gget_save_loc.c ${UNWIND_PATH}/src/s390x/Gglobal.c
    ${UNWIND_PATH}/src/s390x/Ginit.c ${UNWIND_PATH}/src/s390x/Ginit_local.c ${UNWIND_PATH}/src/s390x/Ginit_remote.c
    ${UNWIND_PATH}/src/s390x/Gget_proc_info.c ${UNWIND_PATH}/src/s390x/Gregs.c ${UNWIND_PATH}/src/s390x/Gresume.c
    ${UNWIND_PATH}/src/s390x/Gis_signal_frame.c ${UNWIND_PATH}/src/s390x/Gstep.c
)

# The list of files that go into libunwind and libunwind-loongarch64:
SET(libunwind_la_SOURCES_loongarch_common
    ${libunwind_la_SOURCES_common}
    ${UNWIND_PATH}/src/loongarch64/is_fpreg.c
    ${UNWIND_PATH}/src/loongarch64/regname.c
)

# The list of files that go into libunwind:
SET(libunwind_la_SOURCES_loongarch64
    ${libunwind_la_SOURCES_loongarch_common}
    ${libunwind_la_SOURCES_local}
    ${UNWIND_PATH}/src/loongarch64/Lget_proc_info.c  ${UNWIND_PATH}/src/loongarch64/Linit.c  ${UNWIND_PATH}/src/loongarch64/Lis_signal_frame.c
    ${UNWIND_PATH}/src/loongarch64/Lstep.c
    ${UNWIND_PATH}/src/loongarch64/getcontext.S
    ${UNWIND_PATH}/src/loongarch64/Lget_save_loc.c
    ${UNWIND_PATH}/src/loongarch64/Linit_local.c   ${UNWIND_PATH}/src/loongarch64/Lregs.c
    ${UNWIND_PATH}/src/loongarch64/Lcreate_addr_space.c  ${UNWIND_PATH}/src/loongarch64/Lglobal.c  ${UNWIND_PATH}/src/loongarch64/Linit_remote.c  ${UNWIND_PATH}/src/loongarch64/Lresume.c
)

SET(libunwind_loongarch64_la_SOURCES_loongarch
    ${libunwind_la_SOURCES_loongarch_common}
    ${libunwind_la_SOURCES_generic}
	${UNWIND_PATH}/src/loongarch64/Gcreate_addr_space.c ${UNWIND_PATH}/src/loongarch64/Gget_proc_info.c ${UNWIND_PATH}/src/loongarch64/Gget_save_loc.c
	${UNWIND_PATH}/src/loongarch64/Gglobal.c ${UNWIND_PATH}/src/loongarch64/Ginit.c ${UNWIND_PATH}/src/loongarch64/Ginit_local.c ${UNWIND_PATH}/src/loongarch64/Ginit_remote.c
	${UNWIND_PATH}/src/loongarch64/Gis_signal_frame.c ${UNWIND_PATH}/src/loongarch64/Gregs.c ${UNWIND_PATH}/src/loongarch64/Gresume.c ${UNWIND_PATH}/src/loongarch64/Gstep.c
)

#if(TARGET_AARCH64)
#    SET(libunwind_la_SOURCES                    ${libunwind_la_SOURCES_aarch64})
#    SET(libunwind_remote_la_SOURCES             ${libunwind_aarch64_la_SOURCES_aarch64})
#    SET(libunwind_elf_la_SOURCES                ${libunwind_elf64_la_SOURCES})
#    list(APPEND libunwind_setjmp_la_SOURCES     ${UNWIND_PATH}/src/aarch64/siglongjmp.S)
#elseif(TARGET_ARM)
#    SET(libunwind_la_SOURCES                    ${libunwind_la_SOURCES_arm})
#    SET(libunwind_remote_la_SOURCES             ${libunwind_arm_la_SOURCES_arm})
#    SET(libunwind_elf_la_SOURCES                ${libunwind_elf32_la_SOURCES})
#    list(APPEND libunwind_setjmp_la_SOURCES     ${UNWIND_PATH}/src/arm/siglongjmp.S)
#elseif(TARGET_AMD64)
    SET(libunwind_la_SOURCES                    ${libunwind_la_SOURCES_x86_64})
    SET(libunwind_remote_la_SOURCES             ${libunwind_x86_64_la_SOURCES_x86_64})
    SET(libunwind_elf_la_SOURCES                ${libunwind_elf64_la_SOURCES})
    list(APPEND libunwind_setjmp_la_SOURCES     ${UNWIND_PATH}/src/x86_64/longjmp.S ${UNWIND_PATH}/src/x86_64/siglongjmp.SA)
  #elseif(TARGET_S390X)
  #    SET(libunwind_la_SOURCES                    ${libunwind_la_SOURCES_s390x})
  #    SET(libunwind_remote_la_SOURCES             ${libunwind_s390x_la_SOURCES_s390x})
  #    SET(libunwind_elf_la_SOURCES                ${libunwind_elf64_la_SOURCES})
  #elseif(TARGET_LOONGARCH64)
  #    SET(libunwind_la_SOURCES                    ${libunwind_la_SOURCES_loongarch64})
  #    SET(libunwind_remote_la_SOURCES             ${libunwind_loongarch64_la_SOURCES_loongarch})
  #    SET(libunwind_elf_la_SOURCES                ${libunwind_elf64_la_SOURCES})
  #endif()

set(HAVE_ELF_H 1)
set(HAVE_ENDIAN_H 1)
set(TARGET_AMD64 1)
set(arch x86_64)

add_library(libunwind
OBJECT 
  #${UNWIND_PATH}/src/remote/win/missing-functions.c
  ${libunwind_la_SOURCES} # Local...
  #${libunwind_remote_la_SOURCES}
  # Commented out above for LOCAL + REMOTE runtime build
  ${UNWIND_PATH}/src/mi/Gget_accessors.c
  # ${libunwind_dwarf_local_la_SOURCES}
  #${libunwind_dwarf_common_la_SOURCES}
  #${libunwind_dwarf_generic_la_SOURCES}
  ${libunwind_elf_la_SOURCES}
)
target_compile_definitions(libunwind PRIVATE __x86_64__)
target_compile_definitions(libunwind PRIVATE __amd64__)
target_compile_definitions(libunwind PRIVATE __linux__)
target_compile_definitions(libunwind PRIVATE HAVE_CONFIG_H)
target_compile_definitions(libunwind PRIVATE UNW_REMOTE_ONLY)

target_include_directories(libunwind PUBLIC ${UNWIND_PATH}/include)
target_include_directories(libunwind PUBLIC ${UNWIND_PATH}/include/tdep)
target_include_directories(libunwind PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include/tdep)
target_include_directories(libunwind PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
target_include_directories(libunwind PUBLIC ${UNWIND_PATH}/src)
  

configure_file(${UNWIND_PATH}/include/config.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)
configure_file(${UNWIND_PATH}/include/libunwind-common.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/libunwind-common.h)
configure_file(${UNWIND_PATH}/include/libunwind.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/libunwind.h)
configure_file(${UNWIND_PATH}/include/tdep/libunwind_i.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/tdep/libunwind_i.h)


