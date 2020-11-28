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

OBJS=\
	$(OBJDIR)\guimain.obj\

LIBS=\
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
	/Fo"$(OBJDIR)\\"

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/SUBSYSTEM:WINDOWS\

all: $(OUTDIR)\$(TARGET)

$(OUTDIR)\$(TARGET): $(OBJS)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

clean:
	@if exist $(OBJDIR) $(RD) $(OBJDIR)
	@if exist $(OUTDIR)\$(TARGET) $(RM) $(OUTDIR)\$(TARGET)
	@if exist $(OUTDIR)\$(TARGET:exe=ilk) $(RM) $(OUTDIR)\$(TARGET:exe=ilk)
	@if exist $(OUTDIR)\$(TARGET:exe=pdb) $(RM) $(OUTDIR)\$(TARGET:exe=pdb)
