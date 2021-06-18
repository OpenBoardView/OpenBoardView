#ifndef GENCADFILE_H
#define GENCADFILE_H

#include "BRDFile.h"

#include "GENCADFileGrammar.h"

#include <string>

enum cleanup_length_e
{
#define X(CVAR, NAME) _ignore_me_ ## CVAR,
	X_MACRO_PARSE_VARS
	CLEANUP_LENGTH
#undef X
};


class GenCADFile : public BRDFileBase
{
public:
	GenCADFile(const char *filename);

	enum Dimension {
		INCH,			// Inches.
		THOU,			// for thousandths of an inch.
		MM,				// Millimeters.
		MM100,			// for hundredths of a millimeter.
		USER,			// <p_integer>—number of units to the inch.
		USERCM,			// <p_integer>—number of units to a centimeter.
		USERMM,			// <p_integer>—number of units to a millimeter.
		INVALID,
	};

private:
	enum Dimension m_dimension = INCH;
	int m_dimension_unit = 0;
	bool parse_file(const char* filename);

	bool parse_dimension_units(mpc_ast_t *header_ast);
	bool parse_board_outline(mpc_ast_t *board_ast);
	void add_arc_to_outline(mpc_ast_t *start, mpc_ast_t *stop, mpc_ast_t *center);
	bool parse_vias();
	bool parse_route_vias(mpc_ast_t *route_ast);
	bool parse_components();

	bool parse_shape_pins_to_component(BRDPart *part, double rotation_in_degrees, mpc_ast_t *shape_ast);

	char *get_signal_name_for_component_pin(const char *component_name, mpc_ast_t *pin_ast);
	mpc_ast_t *get_device_by_name(const char *name);
	mpc_ast_t *get_shape_by_name(const char *name);
	char *get_nonquoted_or_quoted_string_child(mpc_ast_t *parent, const char * name);
	char *get_stringtoend_child(mpc_ast_t *parent, const char * name);
	mpc_ast_t *get_padstack_by_name(const char* padstack_name);
	mpc_ast_t *get_pad_by_name(const char* pad_name);
	double get_padstack_radius(mpc_ast_t *padstack_ast);
	double get_pad_radius(mpc_ast_t *pad_ast);


	int board_unit_to_brd_coordinate(double brdUnit);
	bool x_y_ref_to_brd_point(mpc_ast_t *x_y_ref, BRDPoint *point);

	bool is_shape_smd(mpc_ast_t *shape_ast);
	bool is_padstack_smd(mpc_ast_t *padstack_ast);

	mpc_ast_t *header_ast = nullptr;
	mpc_ast_t *board_ast = nullptr;
	mpc_ast_t *signals_ast = nullptr;
	mpc_ast_t *devices_ast = nullptr;
	mpc_ast_t *shapes_ast = nullptr;
	mpc_ast_t *components_ast = nullptr;
	mpc_ast_t *pads_ast = nullptr;
	mpc_ast_t *padstacks_ast = nullptr;
	mpc_ast_t *routes_ast = nullptr;
	mpc_ast_t *tracks_ast = nullptr;
	mpc_ast_t *layers_ast = nullptr;

	int nc_counter = 0;

	double distance(BRDPoint &p1, BRDPoint &p2);

	const double arc_slice_angle_rad = 0.1;
};

#endif // GENCADFILE_H
