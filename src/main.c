#include "ui.h"

/*
 * Dell Inspiron 15 3511 - Intel Advanced UI
 *
 * Generated from the provided IFR dump. This is an independent UEFI
 * Forms-Processor style UI: it renders the discovered forms/questions,
 * reads the corresponding UEFI variables, stages edits in RAM, and writes
 * them only when F10 is pressed.
 *
 * The renderer intentionally keeps the complete discovered question set
 * rather than trimming the UI to only overclocking controls.
 */

static EFI_SYSTEM_TABLE *ST;
static SIMPLE_TEXT_OUTPUT_PROTOCOL *OUTP;
static SIMPLE_TEXT_INPUT_PROTOCOL *INP;
static EFI_RUNTIME_SERVICES *RT;

#define MAX_STORE_BYTES  4096
#define TITLE_ATTR      0x1F
#define NORMAL_ATTR     0x07
#define SELECT_ATTR     0x70
#define VALUE_ATTR      0x0F
#define DIM_ATTR        0x08
#define IA_MAX_VISIBLE  16
#define IA_MAX_LINE     220

static UINT8 store_data[32][MAX_STORE_BYTES];
static UINT32 store_attr[32];
static UINT32 store_len[32];
static UINT8 store_loaded[32];
static UINT8 store_missing[32];
static UINT8 store_dirty[32];
static unsigned store_slot[0x1600]; /* sparse StoreId -> slot, 0xFFFFFFFF means none */
static unsigned store_qcount[32];

static void ascii_to_u16(const char *s, CHAR16 *d, unsigned cap) {
    unsigned i=0;
    if (!cap) return;
    while (s && s[i] && i+1<cap) {
        d[i]=(CHAR16)(unsigned char)s[i];
        i++;
    }
    d[i]=0;
}

static void putc16(CHAR16 c) {
    CHAR16 x[2] = {c,0};
    OUTP->OutputString(OUTP,x);
}

static void outs(const char *s) {
    CHAR16 b[256];
    ascii_to_u16(s,b,256);
    OUTP->OutputString(OUTP,b);
}

static void outu16(const CHAR16 *s) {
    OUTP->OutputString(OUTP,s);
}

static void outs_at(unsigned col, unsigned row, const char *s) {
    OUTP->SetCursorPosition(OUTP,col,row);
    outs(s);
}

static void clear_screen(void) {
    OUTP->SetAttribute(OUTP,NORMAL_ATTR);
    OUTP->ClearScreen(OUTP);
}

static void header(const char *title) {
    OUTP->SetAttribute(OUTP,TITLE_ATTR);
    OUTP->ClearScreen(OUTP);
    OUTP->SetCursorPosition(OUTP,0,0);
    outs(" Intel Advanced  |  Dell Inspiron 15 3511");
    OUTP->SetCursorPosition(OUTP,0,1);
    outs(" ");
    outs(title);
    OUTP->SetAttribute(OUTP,NORMAL_ATTR);
}

static void number_u64(UINT64 v, char *buf) {
    char tmp[32];
    unsigned n=0,i=0;
    if (v==0) { buf[0]='0'; buf[1]=0; return; }
    while(v && n<31) { tmp[n++]=(char)('0'+(v%10)); v/=10; }
    while(n) buf[i++]=tmp[--n];
    buf[i]=0;
}

static void hex_u64(UINT64 v, unsigned digits, char *buf) {
    static const char hx[]="0123456789ABCDEF";
    unsigned i;
    for(i=0;i<digits;i++) {
        unsigned sh=(digits-1-i)*4;
        buf[i]=hx[(v>>sh)&0xF];
    }
    buf[digits]=0;
}

static void append(char *dst, unsigned cap, unsigned *pos, const char *s) {
    if(!s) return;
    while(*s && *pos+1<cap) dst[(*pos)++]=*s++;
    dst[*pos]=0;
}

static void make_value_text(UINT32 qidx, char *buf, unsigned cap) {
    const IA_QUESTION *q=&ia_questions[qidx];
    unsigned slot = (q->StoreId < 0x1600) ? store_slot[q->StoreId] : 0xFFFFFFFFu;
    unsigned i;
    UINT64 v=0;
    if (slot==0xFFFFFFFFu || !store_loaded[slot] || q->Offset+q->Size>store_len[slot]) {
        append(buf,cap,&i,"N/A"); return;
    }
    for(i=0;i<q->Size && i<8;i++) v |= ((UINT64)store_data[slot][q->Offset+i]) << (8*i);

    if(q->Type==3) { /* String */
        unsigned p=0, maxc=q->Size/2;
        for(i=0;i<maxc && q->Offset+2*i+1<store_len[slot] && p+1<cap;i++) {
            UINT16 wc=(UINT16)(store_data[slot][q->Offset+2*i] |
                               ((UINT16)store_data[slot][q->Offset+2*i+1]<<8));
            if(!wc) break;
            if(wc>=32 && wc<127) buf[p++]=(char)wc;
            else buf[p++]='?';
        }
        buf[p]=0;
        return;
    }

    if(q->Type==1 && q->OptionCount) {
        for(i=0;i<q->OptionCount;i++) {
            const IA_OPTION *o=&ia_options[q->OptionBase+i];
            if(o->Value==v) {
                unsigned p=0;
                append(buf,cap,&p,o->Label);
                return;
            }
        }
    }

    if(q->Type==2) {
        append(buf,cap,&i,(v ? "Enabled" : "Disabled"));
        return;
    }

    number_u64(v,buf);
}

static UINT64 get_q_value(const IA_QUESTION *q, UINT8 *ok) {
    unsigned slot = (q->StoreId < 0x1600) ? store_slot[q->StoreId] : 0xFFFFFFFFu;
    UINT64 v=0;
    unsigned i;
    *ok=0;
    if(slot==0xFFFFFFFFu || !store_loaded[slot] || q->Offset+q->Size>store_len[slot]) return 0;
    for(i=0;i<q->Size && i<8;i++) v |= ((UINT64)store_data[slot][q->Offset+i]) << (8*i);
    *ok=1;
    return v;
}

static void set_q_value(const IA_QUESTION *q, UINT64 v) {
    unsigned slot = (q->StoreId < 0x1600) ? store_slot[q->StoreId] : 0xFFFFFFFFu;
    unsigned i;
    if(slot==0xFFFFFFFFu || !store_loaded[slot] || q->Offset+q->Size>store_len[slot]) return;
    for(i=0;i<q->Size && i<8;i++) store_data[slot][q->Offset+i]=(UINT8)((v>>(8*i))&0xFF);
    store_dirty[slot]=1;
}

static void init_store_slots(void) {
    unsigned i;
    for(i=0;i<0x1600;i++) store_slot[i]=0xFFFFFFFFu;
    for(i=0;i<ia_store_count && i<32;i++) {
        if(ia_stores[i].Id < 0x1600) store_slot[ia_stores[i].Id]=i;
        store_qcount[i]=0;
    }
    for(i=0;i<ia_question_count;i++) {
        unsigned slot = (ia_questions[i].StoreId < 0x1600) ? store_slot[ia_questions[i].StoreId] : 0xFFFFFFFFu;
        if(slot<32) store_qcount[slot]++;
    }
}

static EFI_STATUS read_store(unsigned i) {
    CHAR16 name[80];
    UINTN sz=0;
    UINT32 attr=0;
    EFI_STATUS st;
    if(i>=ia_store_count) return EFI_NOT_FOUND;
    ascii_to_u16(ia_stores[i].Name,name,80);
    st=RT->GetVariable(name,(EFI_GUID*)&ia_stores[i].Guid,&attr,&sz,0);
    if(st!=EFI_BUFFER_TOO_SMALL || sz>MAX_STORE_BYTES) {
        store_missing[i]=1;
        store_loaded[i]=0;
        store_len[i]=0;
        return st;
    }
    st=RT->GetVariable(name,(EFI_GUID*)&ia_stores[i].Guid,&attr,&sz,store_data[i]);
    if(EFI_ERROR(st)) {
        store_missing[i]=1; store_loaded[i]=0; store_len[i]=0; return st;
    }
    store_attr[i]=attr; store_len[i]=(UINT32)sz; store_loaded[i]=1; store_missing[i]=0; store_dirty[i]=0;
    return EFI_SUCCESS;
}

static void load_all_stores(void) {
    unsigned i;
    for(i=0;i<32;i++) { store_loaded[i]=0; store_missing[i]=0; store_dirty[i]=0; store_len[i]=0; }
    for(i=0;i<ia_store_count && i<32;i++) read_store(i);
}

static void write_status_line(const char *s) {
    OUTP->SetCursorPosition(OUTP,0,23);
    OUTP->SetAttribute(OUTP,TITLE_ATTR);
    for(unsigned i=0;i<78;i++) putc16(' ');
    OUTP->SetCursorPosition(OUTP,1,23);
    outs(s);
    OUTP->SetAttribute(OUTP,NORMAL_ATTR);
}

static EFI_STATUS save_all(void) {
    EFI_STATUS last=EFI_SUCCESS;
    unsigned i;
    for(i=0;i<ia_store_count && i<32;i++) {
        if(!store_dirty[i] || !store_loaded[i]) continue;
        CHAR16 name[80];
        ascii_to_u16(ia_stores[i].Name,name,80);
        EFI_STATUS st=RT->SetVariable(name,(EFI_GUID*)&ia_stores[i].Guid,
                                      store_attr[i],store_len[i],store_data[i]);
        if(EFI_ERROR(st)) last=st;
        else store_dirty[i]=0;
    }
    return last;
}

static int read_key(EFI_INPUT_KEY *k) {
    EFI_STATUS st=INP->ReadKeyStroke(INP,k);
    return st==EFI_SUCCESS;
}

static void wait_key(void) {
    EFI_INPUT_KEY k;
    while(!read_key(&k)) {}
}

static int is_enter(EFI_INPUT_KEY *k) { return k->UnicodeChar==0x000D || k->UnicodeChar==0x000A; }
static int is_esc(EFI_INPUT_KEY *k) { return k->UnicodeChar==0x001B || k->ScanCode==SCAN_ESC; }

static void edit_numeric(unsigned qidx) {
    const IA_QUESTION *q=&ia_questions[qidx];
    UINT8 ok; UINT64 v=get_q_value(q,&ok);
    EFI_INPUT_KEY k;
    if(!ok) return;
    for(;;) {
        header("Edit value");
        char line[160], val[40], mn[40], mx[40], off[20], qid[20];
        number_u64(v,val); number_u64(q->Min,mn); number_u64(q->Max,mx);
        hex_u64(q->Offset,4,off); hex_u64(q->QuestionId,4,qid);
        outs_at(2,4,q->Prompt);
        line[0]=0; unsigned p=0;
        append(line,160,&p,"Current: "); append(line,160,&p,val);
        append(line,160,&p,"   Range: "); append(line,160,&p,mn);
        append(line,160,&p,".."); append(line,160,&p,mx);
        outs_at(2,6,line);
        line[0]=0;p=0;
        append(line,160,&p,"VarStore "); append(line,160,&p,"0x");
        append(line,160,&p,off); append(line,160,&p,"  QID 0x"); append(line,160,&p,qid);
        outs_at(2,7,line);
        outs_at(2,20,"Left/Right: -/+   Enter: apply   Esc: cancel");
        while(!read_key(&k)){}
        if(is_esc(&k)) return;
        if(is_enter(&k)) { set_q_value(q,v); return; }
        if(k.ScanCode==SCAN_LEFT) { if(v>q->Min) v--; }
        else if(k.ScanCode==SCAN_RIGHT) { if(v<q->Max) v++; }
        else if(k.UnicodeChar>='0' && k.UnicodeChar<='9') {
            /* simple digit-entry: replace with digit */
            UINT64 nv=v*10+(k.UnicodeChar-'0');
            if(nv>=q->Min && nv<=q->Max) v=nv;
        }
    }
}

static void edit_oneof(unsigned qidx) {
    const IA_QUESTION *q=&ia_questions[qidx];
    UINT8 ok; UINT64 v=get_q_value(q,&ok);
    EFI_INPUT_KEY k;
    if(!ok || !q->OptionCount) return;
    unsigned cur=0, i;
    for(i=0;i<q->OptionCount;i++) if(ia_options[q->OptionBase+i].Value==v){cur=i;break;}
    for(;;) {
        header("Select option");
        outs_at(2,4,q->Prompt);
        for(i=0;i<q->OptionCount && i<16;i++) {
            unsigned row=6+i;
            if(i==cur) OUTP->SetAttribute(OUTP,SELECT_ATTR);
            outs_at(4,row,ia_options[q->OptionBase+i].Label);
            if(i==cur) OUTP->SetAttribute(OUTP,NORMAL_ATTR);
        }
        outs_at(2,22,"Left/Right: change   Enter: apply   Esc: cancel");
        while(!read_key(&k)){}
        if(is_esc(&k)) return;
        if(is_enter(&k)) { set_q_value(q,ia_options[q->OptionBase+cur].Value); return; }
        if(k.ScanCode==SCAN_UP || k.ScanCode==SCAN_LEFT) { if(cur==0) cur=q->OptionCount-1; else cur--; }
        if(k.ScanCode==SCAN_DOWN || k.ScanCode==SCAN_RIGHT) { cur++; if(cur>=q->OptionCount)cur=0; }
    }
}

static void edit_checkbox(unsigned qidx) {
    const IA_QUESTION *q=&ia_questions[qidx];
    UINT8 ok; UINT64 v=get_q_value(q,&ok);
    if(ok) set_q_value(q, v?0:1);
}

static void edit_string(unsigned qidx) {
    const IA_QUESTION *q=&ia_questions[qidx];
    unsigned slot=(q->StoreId<0x1600)?store_slot[q->StoreId]:0xFFFFFFFFu;
    if(slot==0xFFFFFFFFu || !store_loaded[slot]) return;
    unsigned maxc=q->Size/2, len=0, i;
    static CHAR16 edit[128];
    if(maxc>127) maxc=127;
    for(i=0;i<maxc && q->Offset+2*i+1<store_len[slot];i++) {
        edit[i]=(CHAR16)(store_data[slot][q->Offset+2*i] | ((UINT16)store_data[slot][q->Offset+2*i+1]<<8));
        if(!edit[i]) break;
        len=i+1;
    }
    edit[len]=0;
    unsigned pos=len;
    EFI_INPUT_KEY k;
    for(;;) {
        header("Edit string");
        outs_at(2,4,q->Prompt);
        OUTP->SetCursorPosition(OUTP,2,7);
        outu16(edit);
        OUTP->SetCursorPosition(OUTP,2+pos,7);
        outs_at(2,20,"Enter: apply   Esc: cancel   Backspace/Delete supported");
        while(!read_key(&k)){}
        if(is_esc(&k)) return;
        if(is_enter(&k)) {
            for(i=0;i<q->Size;i++) store_data[slot][q->Offset+i]=0;
            for(i=0;i<len && 2*i+1<q->Size;i++) {
                store_data[slot][q->Offset+2*i]=(UINT8)(edit[i]&0xFF);
                store_data[slot][q->Offset+2*i+1]=(UINT8)(edit[i]>>8);
            }
            store_dirty[slot]=1;
            return;
        }
        if(k.UnicodeChar==8) {
            if(pos) { pos--; for(i=pos;i<len;i++) edit[i]=edit[i+1]; len--; }
        } else if(k.ScanCode==SCAN_LEFT) { if(pos)pos--; }
        else if(k.ScanCode==SCAN_RIGHT) { if(pos<len)pos++; }
        else if(k.UnicodeChar>=32 && k.UnicodeChar<127 && pos<maxc) {
            for(i=len;i>pos;i--) edit[i]=edit[i-1];
            edit[pos++]=k.UnicodeChar; len++;
            edit[len]=0;
        }
    }
}

static void edit_question(unsigned qidx) {
    const IA_QUESTION *q=&ia_questions[qidx];
    if(q->Type==0) edit_numeric(qidx);
    else if(q->Type==1) edit_oneof(qidx);
    else if(q->Type==2) edit_checkbox(qidx);
    else if(q->Type==3) edit_string(qidx);
}

static void question_line(unsigned qidx, unsigned row, int selected) {
    const IA_QUESTION *q=&ia_questions[qidx];
    char val[80], qh[12], line[220];
    unsigned p=0;
    make_value_text(qidx,val,sizeof(val));
    hex_u64(q->QuestionId,4,qh);
    line[0]=0;
    append(line,sizeof(line),&p,q->Prompt[0]?q->Prompt:"(unnamed)");
    while(p<46) line[p++]=' ';
    line[p]=0;
    append(line,sizeof(line),&p," [");
    append(line,sizeof(line),&p,val);
    append(line,sizeof(line),&p,"]");
    if(selected) OUTP->SetAttribute(OUTP,SELECT_ATTR);
    outs_at(2,row,line);
    if(selected) OUTP->SetAttribute(OUTP,NORMAL_ATTR);
}

static unsigned collect_store_questions(unsigned store_idx, unsigned *out, unsigned cap) {
    unsigned n=0, qi;
    if(store_idx>=ia_store_count) return 0;
    for(qi=0; qi<ia_question_count && n<cap; qi++) {
        if(ia_questions[qi].StoreId==ia_stores[store_idx].Id) out[n++]=qi;
    }
    return n;
}

static void store_view(unsigned si) {
    if(si>=ia_store_count) return;
    static unsigned qidx[3889];
    unsigned count=collect_store_questions(si,qidx,3889);
    UINT32 sel=0, top=0;
    EFI_INPUT_KEY k;
    for(;;) {
        header(ia_stores[si].Name);
        char meta[180], idh[12], szh[12];
        unsigned p=0;
        hex_u64(ia_stores[si].Id,4,idh);
        hex_u64(ia_stores[si].Size,4,szh);
        meta[0]=0;
        append(meta,sizeof(meta),&p,"VarStore 0x");
        append(meta,sizeof(meta),&p,idh);
        append(meta,sizeof(meta),&p,"  Size 0x");
        append(meta,sizeof(meta),&p,szh);
        append(meta,sizeof(meta),&p,"  Questions: ");
        char nn[20]; number_u64(count,nn); append(meta,sizeof(meta),&p,nn);
        outs_at(2,2,meta);

        if(count==0) {
            outs_at(2,5,"This VarStore has no direct variable-backed questions.");
            outs_at(2,7,"Use Esc to return.");
        } else {
            if(sel>=count)sel=count-1;
            if(sel<top)top=sel;
            if(sel>=top+17)top=sel-16;
            unsigned i;
            for(i=0;i<17 && top+i<count;i++) {
                question_line(qidx[top+i],4+i,(top+i)==sel);
            }
            char footer[160]; footer[0]=0; p=0;
            append(footer,sizeof(footer),&p,"Questions: "); append(footer,sizeof(footer),&p,nn);
            append(footer,sizeof(footer),&p,"  F10 Save  F9 Reload  Enter Edit  Esc Back");
            outs_at(2,22,footer);
        }
        while(!read_key(&k)){}
        if(is_esc(&k)) return;
        if(k.ScanCode==SCAN_UP && count) { if(sel==0)sel=count-1; else sel--; }
        else if(k.ScanCode==SCAN_DOWN && count) { sel++; if(sel>=count)sel=0; }
        else if(k.ScanCode==SCAN_PAGEUP && count) { if(sel>16)sel-=16; else sel=0; }
        else if(k.ScanCode==SCAN_PAGEDOWN && count) { sel+=16; if(sel>=count)sel=count-1; }
        else if(k.ScanCode==SCAN_F9) { load_all_stores(); }
        else if(k.ScanCode==SCAN_F10) {
            EFI_STATUS st=save_all();
            if(EFI_ERROR(st)) write_status_line("Save failed. No further variables were written after the first failure.");
            else write_status_line("Saved staged changes to UEFI variables.");
        }
        else if(is_enter(&k) && count) edit_question(qidx[sel]);
    }
}

static void main_view(void) {
    UINT32 sel=0, top=0;
    EFI_INPUT_KEY k;
    for(;;) {
        header("Intel Advanced | VarStores");
        if(sel>=ia_store_count)sel=0;
        if(sel<top)top=sel;
        if(sel>=top+17)top=sel-16;
        unsigned i;
        for(i=0;i<17 && top+i<ia_store_count;i++) {
            unsigned idx=top+i;
            char line[180], idh[12], szh[12], cnt[20]; unsigned p=0;
            unsigned qcount=store_qcount[idx];
            hex_u64(ia_stores[idx].Id,4,idh);
            hex_u64(ia_stores[idx].Size,4,szh);
            number_u64(qcount,cnt);
            line[0]=0;
            append(line,sizeof(line),&p,ia_stores[idx].Name);
            while(p<30)line[p++]=' ';
            append(line,sizeof(line),&p," [0x"); append(line,sizeof(line),&p,idh);
            append(line,sizeof(line),&p,"] size=0x"); append(line,sizeof(line),&p,szh);
            append(line,sizeof(line),&p," q="); append(line,sizeof(line),&p,cnt);
            if(idx==sel)OUTP->SetAttribute(OUTP,SELECT_ATTR);
            outs_at(2,4+i,line);
            if(idx==sel)OUTP->SetAttribute(OUTP,NORMAL_ATTR);
        }
        outs_at(2,22,"↑↓ Select  PgUp/PgDn Page  Enter Open  F9 Reload  F10 Save  Esc Exit");
        outs_at(2,23,"20 VarStores / 3889 IFR-derived variable-backed questions");
        while(!read_key(&k)){}
        if(is_esc(&k)) return;
        if(k.ScanCode==SCAN_UP) { if(sel==0)sel=ia_store_count-1; else sel--; }
        else if(k.ScanCode==SCAN_DOWN) { sel++; if(sel>=ia_store_count)sel=0; }
        else if(k.ScanCode==SCAN_PAGEUP) { if(sel>16)sel-=16; else sel=0; }
        else if(k.ScanCode==SCAN_PAGEDOWN) { sel+=16; if(sel>=ia_store_count)sel=ia_store_count-1; }
        else if(k.ScanCode==SCAN_F9) { load_all_stores(); }
        else if(k.ScanCode==SCAN_F10) {
            EFI_STATUS st=save_all();
            if(EFI_ERROR(st)) write_status_line("Save failed.");
            else write_status_line("Saved staged changes.");
        }
        else if(is_enter(&k)) store_view(sel);
    }
}

UINT64 efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle;
    ST=SystemTable;
    OUTP=ST->ConOut;
    INP=ST->ConIn;
    RT=ST->RuntimeServices;
    OUTP->SetAttribute(OUTP,NORMAL_ATTR);
    init_store_slots();
    load_all_stores();

    header("Intel Advanced");
    outs_at(2,4,"Dell Inspiron 15 3511");
    outs_at(2,6,"Loading IFR-derived configuration UI...");
    outs_at(2,8,"Setup store : EC87D643-EBA4-4BB5-A1E5-3F3E36B20DA9");
    outs_at(2,9,"CpuSetup    : B08F97FF-E6E8-4193-A997-5E9E9B0ADB32");
    outs_at(2,11,"All discovered VarStores and variable-backed questions are included.");
    outs_at(2,14,"Press any key...");
    wait_key();

    main_view();

    clear_screen();
    OUTP->SetAttribute(OUTP,NORMAL_ATTR);
    outs_at(2,2,"Intel Advanced UI exited.");
    outs_at(2,4,"Changes are staged only until F10 Save is pressed.");
    return EFI_SUCCESS;
}
