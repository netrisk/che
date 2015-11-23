#include <che_cycle.h>
#include <che_log.h>
#include <che_rand.h>

/* To check if a key is pressed */
#define CHE_KEY_PRESSED(_keymask, _key) ((_keymask >> _key) & 1)

/* #define CHE_DBG_OPCODES */

/* macros to get some fields from opcodes */

/* Get the NN from opcodes that use it.
 * ie. 6XNN
 */
#define CHE_GET_OPCODE_NN(_opcode) (uint8_t)(_opcode & 0x00FF)

#define CHE_GET_OPCODE_NNN(_opcode) (uint16_t)(_opcode & 0x0FFF)

#define CHE_GET_NIBBLE_0(_opcode) (_opcode & 0xf)
#define CHE_GET_NIBBLE_1(_opcode) ((_opcode >> 4) & 0xf)
#define CHE_GET_NIBBLE_2(_opcode) ((_opcode >> 8) & 0xf)
#define CHE_GET_NIBBLE_3(_opcode) ((_opcode >> 12) & 0xf)

/* Get the X from opcodes of type:
 * ?X??
 * i.e 6XNN -> Extract the value of X
 */
#define CHE_GET_OPCODE_X(_opcode) CHE_GET_NIBBLE_2(_opcode)
/* Get the Y from opcodes of type:
 * ?X??
 * i.e 8XY0 -> Extract the value of Y
 */
#define CHE_GET_OPCODE_Y(_opcode) CHE_GET_NIBBLE_1(_opcode)

#define CHE_VF(_regs) _regs.v[15]
#define CHE_V0(_regs) _regs.v[0]

#define CHE_NEXT_INSTRUCTION(_pc) _pc+=2
#define CHE_SKIP_NEXT_INSTRUCTION(_pc) _pc+=4

static void che_cycle_unrecognized(che_machine_t *m, uint16_t opcode)
{
	che_log("ERROR: Unrecognized opcode %04X at 0x%03X\n", opcode, m->pc);
}

typedef int (*che_cycle_function_t)(che_machine_t *m, uint16_t opcode);

static int che_cycle_function_0(che_machine_t *m, uint16_t opcode)
{
	if (opcode == 0x00EE) {
		/* Return from subroutine */
		if (m->sp == 0) {
			che_log("ERROR: Empty stack\n");
			return -2;
		}
		m->sp--;
		m->pc = m->stack[m->sp];
	} else if (opcode == 0x00E0) {
		/* Clear the screen */
		che_io_scr_clear(m->io);
		CHE_NEXT_INSTRUCTION(m->pc);
	} else {
		che_log("WARNING: ignoring RCA 1802 call to 0x%03X", opcode);
		CHE_NEXT_INSTRUCTION(m->pc);
	}
	return 0;
}

static int che_cycle_function_1(che_machine_t *m, uint16_t opcode)
{
	/* 1NNN: Jump to NNN */
	m->pc = opcode & 0xfff;
	#ifdef CHE_DBG_OPCODES
	che_log("Jumping to address=%x",m->pc);
	#endif /* CHE_DBG_OPCODES */
	return 0;
}

static int che_cycle_function_2(che_machine_t *m, uint16_t opcode)
{
	/* 2NNN: Call NNN Sub */
	if (m->sp >= CHE_MACHINE_STACK_LEVELS) {
		che_log("ERROR: Stack overflow\n");
		return -1;
	}
	m->stack[m->sp++] = m->pc + 2;
	m->pc = opcode & 0xfff;
	return 0;
}

static int che_cycle_function_3(che_machine_t *m, uint16_t opcode)
{
	/* 3XNN skips the next instruction if VX=NN*/
	#ifdef CHE_DBG_OPCODES
	che_log("Skip next instruction if register[%u] == %x",
	        CHE_GET_OPCODE_X(opcode),
	        CHE_GET_OPCODE_NN(opcode));
	#endif /* CHE_DBG_OPCODES */
	if( m->r.v[CHE_GET_OPCODE_X(opcode)] == CHE_GET_OPCODE_NN(opcode) ) {
		#ifdef CHE_DBG_OPCODES
		che_log("Skipping next instruction");
		#endif /* CHE_DBG_OPCODES */
		CHE_SKIP_NEXT_INSTRUCTION(m->pc);
	} else {
		#ifdef CHE_DBG_OPCODES
		che_log("Not skipping next instruction");
		#endif /* CHE_DBG_OPCODES */
		CHE_NEXT_INSTRUCTION(m->pc);
	}
	return 0;
}

static int che_cycle_function_4(che_machine_t *m, uint16_t opcode)
{
	/* 4XNN Skip the next instruction if VX != NN */
	#ifdef CHE_DBG_OPCODES
	che_log("Skip next instruction if register[%u] != %x",
	        CHE_GET_OPCODE_X(opcode),
	        CHE_GET_OPCODE_NN(opcode));
	#endif /* CHE_DBG_OPCODES */
	if( m->r.v[CHE_GET_OPCODE_X(opcode)] != CHE_GET_OPCODE_NN(opcode) ) {
		#ifdef CHE_DBG_OPCODES
			che_log("Skipping next instruction");
			#endif /* CHE_DBG_OPCODES */
		CHE_SKIP_NEXT_INSTRUCTION(m->pc);
	} else {
		#ifdef CHE_DBG_OPCODES
		che_log("Not skipping next instruction");
		#endif /* CHE_DBG_OPCODES */
		CHE_NEXT_INSTRUCTION(m->pc);
	}
	return 0;
}

static int che_cycle_function_5(che_machine_t *m, uint16_t opcode)
{
	/* 5XY0 Skips the next instruction if VX equals VY. */
	if (m->r.v[CHE_GET_OPCODE_X(opcode)] == m->r.v[CHE_GET_OPCODE_Y(opcode)])
		CHE_SKIP_NEXT_INSTRUCTION(m->pc);
	else
		CHE_NEXT_INSTRUCTION(m->pc);
	return 0;
}

static int che_cycle_function_6(che_machine_t *m, uint16_t opcode)
{
	/* 6XNN Set VX register to NN value */
	m->r.v[CHE_GET_OPCODE_X(opcode)] = CHE_GET_OPCODE_NN(opcode);
	#ifdef CHE_DBG_OPCODES
	che_log("setting register[%u] to value:%x",CHE_GET_OPCODE_X(opcode),
	        CHE_GET_OPCODE_NN(opcode));
	#endif /* CHE_DBG_OPCODES */
	CHE_NEXT_INSTRUCTION(m->pc);
	return 0;
}

static int che_cycle_function_7(che_machine_t *m, uint16_t opcode)
{
	/* 7XNN Adds NN to VX */
	/* TODO: borrow? */
	m->r.v[CHE_GET_OPCODE_X(opcode)] += CHE_GET_OPCODE_NN(opcode);
	CHE_NEXT_INSTRUCTION(m->pc);
	return 0;
}

static int che_cycle_function_8(che_machine_t *m, uint16_t opcode)
{
	switch (CHE_GET_NIBBLE_0(opcode))
	{
	case 0: /* 8XY0 sets Vx to Vy */
		m->r.v[CHE_GET_OPCODE_X(opcode)] = m->r.v[CHE_GET_OPCODE_Y(opcode)];
		break;
	case 1: /* 8xy1 sets Vx to Vx OR Vy */
		m->r.v[CHE_GET_OPCODE_X(opcode)] |= m->r.v[CHE_GET_OPCODE_Y(opcode)];
		break;
	case 2: /* 8xy2 Set Vx to Vx AND Vy */
		m->r.v[CHE_GET_OPCODE_X(opcode)] &= m->r.v[CHE_GET_OPCODE_Y(opcode)];
		break;
	case 3: /* XOR */
		m->r.v[CHE_GET_OPCODE_X(opcode)] ^= m->r.v[CHE_GET_OPCODE_Y(opcode)];
		break;
	case 4: { /* 8xy4 Adds Vy To Vx.Vf to 1 when there's a carry 
	             and 0 when not.*/ 
		uint8_t vy = m->r.v[CHE_GET_OPCODE_Y(opcode)];
		uint16_t vx = m->r.v[CHE_GET_OPCODE_X(opcode)];
			vx+=vy;
			if( vx > 0xff ) {
				CHE_VF(m->r) = 1; /* carry present */
			}
			m->r.v[CHE_GET_OPCODE_X(opcode)] = (uint8_t)(vx & 0x00FF);
		}
		break;
	case 5: { /* 8xy5 Vy is substracted from vx. Vf set to 0 when 
	             borrow, and 1 when not. */
		uint8_t vy = m->r.v[CHE_GET_OPCODE_Y(opcode)];
		int16_t vx = m->r.v[CHE_GET_OPCODE_X(opcode)];
		vx -= vy; /* ¿should I leave to zero of to absolute
		             plus vf flag */
		if( vx < 0 ) {
			CHE_VF(m->r) = 0;
			vx = -vx; /* remove the sign */
		} else {
			CHE_VF(m->r) = 1;
		}
		m->r.v[CHE_GET_OPCODE_X(opcode)] = (uint8_t)(vx & 0x00FF);
		}
		break;
	case 6: /* 8xy6 shifts VX right by one.
	           Vf is set to the value of the lsb of vx before the 
	           shift */
		CHE_VF(m->r) = m->r.v[CHE_GET_OPCODE_X(opcode)] & 0x01;
		m->r.v[CHE_GET_OPCODE_X(opcode)] >>= 1;
		break;
	case 7: {/* 8xy7 sets vx to vy minus vx. Vf set to 0 when there's 
	          a borrow and 1 when there isn't */
		uint8_t vy = m->r.v[CHE_GET_OPCODE_Y(opcode)];
		uint8_t vx = m->r.v[CHE_GET_OPCODE_X(opcode)];
		int16_t res = vy - vx;
		if( res > 0 ) {
			CHE_VF(m->r) = 1;
		} else {
			CHE_VF(m->r) = 0;
			res = -res; /* discard the sign */
		}
		m->r.v[CHE_GET_OPCODE_X(opcode)] = (uint8_t)(res & 0x00FF);
		}
		break;
	case 0xe: /* 8xye shifts vx left by one.VF is set to the value of the 
	             msb of vx before the shift */ 
		CHE_VF(m->r) = m->r.v[CHE_GET_OPCODE_X(opcode)] & 0x80;
		m->r.v[CHE_GET_OPCODE_X(opcode)] <<= 1;
		break;
	default:
		che_cycle_unrecognized(m, opcode);
		return -1;
	}
	CHE_NEXT_INSTRUCTION(m->pc);
	return 0;
}

static int che_cycle_function_9(che_machine_t *m, uint16_t opcode)
{
	/* 9XY0 Skips the next instruction if VX doesn't equal VY */
	if (m->r.v[CHE_GET_OPCODE_X(opcode)] != m->r.v[CHE_GET_OPCODE_Y(opcode)])
		CHE_SKIP_NEXT_INSTRUCTION(m->pc);
	else
		CHE_NEXT_INSTRUCTION(m->pc);
	return 0;
}

static int che_cycle_function_a(che_machine_t *m, uint16_t opcode)
{
	/* ANNN Sets I to the address NNN */
	m->r.i = CHE_GET_OPCODE_NNN(opcode);
	CHE_NEXT_INSTRUCTION(m->pc);
	return 0;
}

static int che_cycle_function_b(che_machine_t *m, uint16_t opcode)
{
	/* BNNN Jumps to the address NNN plus V0 */
	m->pc = CHE_V0(m->r) + CHE_GET_OPCODE_NNN(opcode);
	return 0;
}

static int che_cycle_function_c(che_machine_t *m, uint16_t opcode)
{
	/* CXNN Sets VX to the result of a bitwise and operation on a
	   random number and NN */
	m->r.v[CHE_GET_OPCODE_X(opcode)] = che_rand(&m->rand) &
	                                   CHE_GET_OPCODE_NN(opcode);
	CHE_NEXT_INSTRUCTION(m->pc);
	return 0;
}

static int che_cycle_function_d(che_machine_t *m, uint16_t opcode)
{
	/* DXYN: Draw sprite located at I of height N at X, Y */
	int height = CHE_GET_NIBBLE_0(opcode);
	int x = m->r.v[CHE_GET_NIBBLE_2(opcode)];
	int y = m->r.v[CHE_GET_NIBBLE_1(opcode)];
	m->r.v[0xf] = che_io_scr_sprite(m->io, m->mem + m->r.i, height, x, y);
	CHE_NEXT_INSTRUCTION(m->pc);
	return 0;
}

static int che_cycle_function_e(che_machine_t *m, uint16_t opcode)
{
	/* EX9E and EXA1: Skip instructions when keys are pressed or not */
	uint8_t lowest_byte = opcode & 0xff;
	bool skip;

	/* Get key */
	int key = m->r.v[CHE_GET_OPCODE_X(opcode)];
	if (key > 0xf) {
		che_log("ERROR: Non existing key %d", key);
		return -1;
	}
	/* Check action */
	if (lowest_byte == 0x9e) {
		/* Skips the next instruction if the key stored in VX is pressed */
		skip = CHE_KEY_PRESSED(m->keymask, key);
	} else if (lowest_byte == 0xa1) {
		/* Skips the next instruction if the key stored in VX isn't pressed */
		skip = !CHE_KEY_PRESSED(m->keymask, key);
	} else {
		che_cycle_unrecognized(m, opcode);
		return -1;
	}
	/* Operate */
	if (skip)
		CHE_SKIP_NEXT_INSTRUCTION(m->pc);
	else
		CHE_NEXT_INSTRUCTION(m->pc);
	return 0;
}

static int che_cycle_function_f(che_machine_t *m, uint16_t opcode)
{
	uint8_t lowest_byte = opcode & 0xff;
	/* Deal with timers */
	if (lowest_byte == 0x07) {
		m->r.v[CHE_GET_OPCODE_X(opcode)] = m->delay_timer;
	} else if (lowest_byte == 0x0a) {
		/* A key press is awaited, and then stored in VX */
		/* TODO: actually this doesn't wait for a keypress, just avoids
		         going to next instruction if a key isn't pressed,
		         which consumes more CPU than doing it correctly */
		if (!m->keymask)
			return 0;
		int i;
		for (i = 0; i < 16; i++) {
			if (CHE_KEY_PRESSED(m->keymask, i)) {
				m->r.v[CHE_GET_OPCODE_X(opcode)] = i;
				break;
			}
		}
	} else if (lowest_byte == 0x15) {
		m->delay_timer = m->r.v[CHE_GET_OPCODE_X(opcode)];
	} else if (lowest_byte == 0x18) {
		m->sound_timer = m->r.v[CHE_GET_OPCODE_X(opcode)];
	} else if (lowest_byte == 0x1e) {
		/* Adds VX to I. [3] VF is set to 1 when range overflow
		   (I+VX>0xFFF), and 0 when there isn't.
		   This is undocumented feature of the CHIP-8 and used by
		   Spacefight 2091! game*/
		m->r.i += m->r.v[CHE_GET_OPCODE_X(opcode)];
		if (m->r.i > 0xfff) {
			m->r.i &= 0xfff;
			CHE_VF(m->r) = 1;
		} else {
			CHE_VF(m->r) = 0;
		}
	} else if (lowest_byte == 0x29) {
		/* Sets I to the location of the sprite for the character in VX */
		int value = m->r.v[CHE_GET_OPCODE_X(opcode)];
		if (value > 0xf) {
			che_log("ERROR: Character 0x%02X not in table", value);
			return -1;
		}
		m->r.i = CHE_MACHINE_CHAR_TABLE_POS +
		         sizeof(che_machine_char_t) * value;
	} else if (lowest_byte == 0x33) {
		/* Stores the Binary-coded decimal representation of VX */
		int value = m->r.v[CHE_GET_OPCODE_X(opcode)];
		uint8_t *ptr = m->mem + m->r.i;
		*ptr++ = value / 100;
		*ptr++ = (value % 100) / 10;
		*ptr++ = value % 10;
	} else if (lowest_byte == 0x55) {
		/* Stores V0 to VX in memory starting at address I */
		uint8_t *ptr = m->mem + m->r.i;
		int final_v = CHE_GET_OPCODE_X(opcode);
		int i;
		for (i = 0; i <= final_v; i++)
			*ptr++ = m->r.v[i];
	} else if (lowest_byte == 0x65) {
		/* Stores V0 to VX in memory starting at address I */
		uint8_t *ptr = m->mem + m->r.i;
		int final_v = CHE_GET_OPCODE_X(opcode);
		int i;
		for (i = 0; i <= final_v; i++)
			m->r.v[i] = *ptr++;
	} else {
		che_cycle_unrecognized(m, opcode);
		return -1;
	}
	CHE_NEXT_INSTRUCTION(m->pc);
	return 0;
}

static const che_cycle_function_t che_cycle_function_table[16] = {
	che_cycle_function_0,
	che_cycle_function_1,
	che_cycle_function_2,
	che_cycle_function_3,
	che_cycle_function_4,
	che_cycle_function_5,
	che_cycle_function_6,
	che_cycle_function_7,
	che_cycle_function_8,
	che_cycle_function_9,
	che_cycle_function_a,
	che_cycle_function_b,
	che_cycle_function_c,
	che_cycle_function_d,
	che_cycle_function_e,
	che_cycle_function_f
};

int che_cycle(che_machine_t *m)
{
	uint16_t opcode = m->mem[m->pc] << 8 | m->mem[m->pc + 1];
	#ifdef CHE_DBG_OPCODES
	che_log("opcode=%04x",opcode);
	#endif /* CHE_DBG_OPCODES */

	int func_num = CHE_GET_NIBBLE_3(opcode);
	return che_cycle_function_table[func_num](m, opcode);
}
