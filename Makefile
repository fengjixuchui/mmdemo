!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSE
ARCH=x86
!ENDIF

OUTDIR=bin\$(ARCH)
OBJDIR=obj\$(ARCH)
SRCDIR=src

CC=cl
RD=rd/s/q
RM=del/q
LINKER=link
TARGET=g.exe
TARGET1=t.exe
TARGET2=u.exe

OBJS=\
	$(OBJDIR)\common.obj\
	$(OBJDIR)\guimain.obj\

OBJS1=\
	$(OBJDIR)\common.obj\
	$(OBJDIR)\main1.obj\
	$(OBJDIR)\largepage.obj\

OBJS2=\
	$(OBJDIR)\common.obj\
	$(OBJDIR)\main2.obj\

LIBS=\
	advapi32.lib\
	user32.lib\

CFLAGS=\
	/nologo\
	/c\
	/Od\
	/W4\
	/Zi\
	/EHsc\
	/DUNICODE\
	/D_UNICODE\
	/D_CRT_SECURE_NO_WARNINGS\
	/Fd"$(OBJDIR)\\"\
	/Fo"$(OBJDIR)\\"\

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/SUBSYSTEM:WINDOWS\

LFLAGS_CUI=\
	/NOLOGO\
	/DEBUG\
	/SUBSYSTEM:CONSOLE\

all: $(OUTDIR)\$(TARGET) $(OUTDIR)\$(TARGET1) $(OUTDIR)\$(TARGET2)

$(OUTDIR)\$(TARGET): $(OBJS)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(TARGET1): $(OBJS1)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS_CUI) $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(TARGET2): $(OBJS2)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS_CUI) $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

clean:
	@if exist $(OBJDIR) $(RD) $(OBJDIR)
	@if exist $(OUTDIR)\$(TARGET) $(RM) $(OUTDIR)\$(TARGET)
	@if exist $(OUTDIR)\$(TARGET:exe=ilk) $(RM) $(OUTDIR)\$(TARGET:exe=ilk)
	@if exist $(OUTDIR)\$(TARGET:exe=pdb) $(RM) $(OUTDIR)\$(TARGET:exe=pdb)
	@if exist $(OUTDIR)\$(TARGET1) $(RM) $(OUTDIR)\$(TARGET1)
	@if exist $(OUTDIR)\$(TARGET1:exe=ilk) $(RM) $(OUTDIR)\$(TARGET1:exe=ilk)
	@if exist $(OUTDIR)\$(TARGET1:exe=pdb) $(RM) $(OUTDIR)\$(TARGET1:exe=pdb)
	@if exist $(OUTDIR)\$(TARGET2) $(RM) $(OUTDIR)\$(TARGET2)
	@if exist $(OUTDIR)\$(TARGET2:exe=ilk) $(RM) $(OUTDIR)\$(TARGET2:exe=ilk)
	@if exist $(OUTDIR)\$(TARGET2:exe=pdb) $(RM) $(OUTDIR)\$(TARGET2:exe=pdb)
