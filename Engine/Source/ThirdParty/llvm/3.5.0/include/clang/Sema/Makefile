CLANG_LEVEL := ../../..
TD_SRC_DIR = $(PROJ_SRC_DIR)/../Basic
BUILT_SOURCES = AttrTemplateInstantiate.inc AttrParsedAttrList.inc AttrParsedAttrKinds.inc \
        AttrSpellingListIndex.inc AttrParsedAttrImpl.inc

TABLEGEN_INC_FILES_COMMON = 1

include $(CLANG_LEVEL)/Makefile

$(ObjDir)/AttrTemplateInstantiate.inc.tmp : $(TD_SRC_DIR)/Attr.td \
                                            $(CLANG_TBLGEN) $(ObjDir)/.dir
	$(Echo) "Building Clang attribute template instantiate code with tablegen"
	$(Verb) $(ClangTableGen) -gen-clang-attr-template-instantiate -o \
	  $(call SYSPATH, $@) -I $(PROJ_SRC_DIR)/../../ $<
	  
$(ObjDir)/AttrParsedAttrList.inc.tmp : $(TD_SRC_DIR)/Attr.td \
                                       $(CLANG_TBLGEN) $(ObjDir)/.dir
	$(Echo) "Building Clang parsed attribute list with tablegen"
	$(Verb) $(ClangTableGen) -gen-clang-attr-parsed-attr-list -o \
	  $(call SYSPATH, $@) -I $(PROJ_SRC_DIR)/../../ $<
	  
$(ObjDir)/AttrParsedAttrKinds.inc.tmp : $(TD_SRC_DIR)/Attr.td \
                                       $(CLANG_TBLGEN) $(ObjDir)/.dir
	$(Echo) "Building Clang parsed attribute kinds with tablegen"
	$(Verb) $(ClangTableGen) -gen-clang-attr-parsed-attr-kinds -o \
	  $(call SYSPATH, $@) -I $(PROJ_SRC_DIR)/../../ $<

$(ObjDir)/AttrSpellingListIndex.inc.tmp : $(TD_SRC_DIR)/Attr.td \
                                       $(CLANG_TBLGEN) $(ObjDir)/.dir
	$(Echo) "Building Clang attribute spelling list index with tablegen"
	$(Verb) $(ClangTableGen) -gen-clang-attr-spelling-index -o \
	  $(call SYSPATH, $@) -I $(PROJ_SRC_DIR)/../../ $<

$(ObjDir)/AttrParsedAttrImpl.inc.tmp : $(TD_SRC_DIR)/Attr.td \
                                       $(CLANG_TBLGEN) $(ObjDir)/.dir
	$(Echo) "Building Clang parsed attribute list impl with tablegen"
	$(Verb) $(ClangTableGen) -gen-clang-attr-parsed-attr-impl -o \
	  $(call SYSPATH, $@) -I $(PROJ_SRC_DIR)/../../ $<

