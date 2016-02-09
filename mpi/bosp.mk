
ifdef CONFIG_BBQUE_MPI

# Targets provided by this project
.PHONY: mpirun clean_mpirun

# Add this to the "contrib_testing" target
testing: mpirun
clean_testing: clean_mpirun

MODULE_MPIRUN=barbeque/mpi

mpirun: external
	@echo
	@echo "==== Building bbque-mpirun ($(BUILD_TYPE)) ===="
	@echo " Using GCC    : $(CC)"
	@echo " Target flags : $(TARGET_FLAGS)"
	@echo " Sysroot      : $(PLATFORM_SYSROOT)"
	@echo " BOSP Options : $(CMAKE_COMMON_OPTIONS)"
	@[ -d $(MODULE_MPIRUN)/build/$(BUILD_TYPE) ] || \
		mkdir -p $(MODULE_MPIRUN)/build/$(BUILD_TYPE) || \
		exit 1
	@cd $(MODULE_MPIRUN)/build/$(BUILD_TYPE) && \
		CC=$(CC) CFLAGS=$(TARGET_FLAGS) \
		CXX=$(CXX) CXXFLAGS=$(TARGET_FLAGS) \
		cmake $(CMAKE_COMMON_OPTIONS) ../.. || \
		exit 1
	@cd $(MODULE_MPIRUN)/build/$(BUILD_TYPE) && \
		make -j$(CPUS) install || \
		exit 1

clean_mpirun:
	@echo
	@echo "==== Clean-up MpiRun Application ===="
	@[ ! -f $(BUILD_DIR)/usr/bin/bbque-mpirun ] || \
		rm -f $(BUILD_DIR)/etc/bbque/recipes/MpiRun*; \
		rm -f $(BUILD_DIR)/usr/bin/bbque-mpirun*
	@rm -rf $(MODULE_MPIRUN)/build
	@echo

endif # CONFIG_BBQUE_MPI

