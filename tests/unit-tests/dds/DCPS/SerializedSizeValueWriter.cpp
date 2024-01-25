#include <SerializedSizeValueWriterTypeSupportImpl.h>

#include <dds/DCPS/SerializedSizeValueWriter.h>
#include <dds/DCPS/XTypes/DynamicDataImpl.h>
#include <dds/DCPS/XTypes/DynamicDataAdapter.h>

#include <gtest/gtest.h>

using namespace OpenDDS;
using namespace Test;

DCPS::Encoding xcdr2(DCPS::Encoding::KIND_XCDR2, DCPS::ENDIAN_BIG);

void dump_sizes(const std::vector<size_t>& sizes)
{
  std::cout << "[ ";
  for (size_t i = 0; i < sizes.size(); ++i) {
    std::cout << sizes[i] << " ";
  }
  std::cout << "]" << std::endl;
}

template <typename BaseStructType>
void init_base_struct(BaseStructType& bst)
{
  bst.b_field = true;
  bst.f_field = 1.0f;
  bst.o_field = 0x01;
}

template <typename StructType>
void init(StructType& st)
{
  st.s_field = 10;
  st.l_field = 20;
  init_base_struct(st.nested_field);
  st.str_field = "hello";
  st.ull_field = 30;
}

template <typename Xtag, typename StructType>
void check_total_size(DCPS::SerializedSizeValueWriter& value_writer, const StructType& sample)
{
  // Serialized size returned from serialized_size.
  size_t expected_size = 0;
  DCPS::serialized_size(xcdr2, expected_size, sample);

  // Serialized size computed by vwrite with the C++ object.
  vwrite(value_writer, sample);
  EXPECT_EQ(expected_size, value_writer.get_serialized_size());

  // Serialized size computed by vwrite with a dynamic data object.
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<Xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<Xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());
  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());
  DDS::DynamicData_var dd = XTypes::get_dynamic_data_adapter<StructType, StructType>(dt, sample);

  vwrite(value_writer, dd.in());
  EXPECT_EQ(expected_size, value_writer.get_serialized_size());
}

void check_component_sizes(DCPS::SerializedSizeValueWriter& value_writer, const size_t* arr, size_t length)
{
  std::vector<size_t> expected_sizes(arr, arr + length);
  const std::vector<size_t>& sizes = value_writer.get_serialized_sizes();
  EXPECT_TRUE(expected_sizes == sizes);
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalFinalStruct)
{
  FinalFinalStruct ffs;
  init(ffs);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalFinalStruct_xtag>(value_writer, ffs);

  // Layout           Total size
  // 2: s_field       2
  // 2+4: l_field     8
  // [ nested_field
  // 1: b_field       9
  // 3+4: f_field     16
  // 1: o_field       17
  // nested_field ]
  // 3+4+6: str_field 30
  // 2+8: ull_field   (40)
  const size_t arr[] = {40};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalAppendableStruct)
{
  FinalAppendableStruct fas;
  init(fas);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalAppendableStruct_xtag>(value_writer, fas);

  // Layout           Nested    Total size
  // 2: s_field                 2
  // 2+4: l_field               8
  // [ nested_field
  // 4: Dheader       4         12
  // 1: b_field       5         13
  // 3+4: f_field     12        20
  // 1: o_field       (13)      21
  // nested_field ]
  // 3+4+6: str_field           34
  // 2+8: ull_field             (44)
  const size_t arr[] = {44, 13};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalMutableStruct)
{
  FinalMutableStruct fms;
  init(fms);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalMutableStruct_xtag>(value_writer, fms);

  // Layout           Nested    Total size
  // 2: s_field                 2
  // 2+4: l_field               8
  // [ nested_field
  // 4: Dheader       4         12
  // 4: Emheader      8         16
  // 1: b_field       9         17
  // 3+4: Emheader    16        24
  // 4: f_field       20        28
  // 4: Emheader      24        32
  // 1: o_field       (25)      33
  // nested_field ]
  // 3+4+6: str_field           46
  // 2+8: ull_field             (56)
  const size_t arr[] = {56, 25, 1, 4, 1};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableFinalStruct)
{
  AppendableFinalStruct afs;
  init(afs);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableFinalStruct_xtag>(value_writer, afs);

  // Layout           Total size
  // 4: Dheader       4
  // 2: s_field       6
  // 2+4: l_field     12
  // [ nested_field
  // 1: b_field       13
  // 3+4: f_field     20
  // 1: o_field       21
  // nested_field ]
  // 3+4+6: str_field 34
  // 2+8: ull_field   (44)
  const size_t arr[] = {44};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableAppendableStruct)
{
  AppendableAppendableStruct aas;
  init(aas);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableAppendableStruct_xtag>(value_writer, aas);

  // Layout           Nested    Total size
  // 4: Dheader                 4
  // 2: s_field                 6
  // 2+4: l_field               12
  // [ nested_field
  // 4: Dheader       4         16
  // 1: b_field       5         17
  // 3+4: f_field     12        24
  // 1: o_field       (13)      25
  // nested_field ]
  // 3+4+6: str_field           38
  // 2+8: ull_field             (48)
  const size_t arr[] = {48, 13};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableMutableStruct)
{
  AppendableMutableStruct ams;
  init(ams);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableMutableStruct_xtag>(value_writer, ams);

  // Layout           Nested    Total size
  // 4: Dheader                 4
  // 2: s_field                 6
  // 2+4: l_field               12
  // [ nested_field
  // 4: Dheader       4         16
  // 4: Emheader      8         20
  // 1: b_field       9         21
  // 3+4: Emheader    16        28
  // 4: f_field       20        32
  // 4: Emheader      24        36
  // 1: o_field       (25)      37
  // nested_field ]
  // 3+4+6: str_field           50
  // 2+8: ull_field             (60)
  const size_t arr[] = {60, 25, 1, 4, 1};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableFinalStruct)
{
  MutableFinalStruct mfs;
  init(mfs);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableFinalStruct_xtag>(value_writer, mfs);

  // Layout           Nested    Total size
  // 4: Dheader                 4
  // 4: Emheader                8
  // 2: s_field                 10
  // 2+4: Emheader              16
  // 4: l_field                 20
  // 4: Emheader                24
  // 4: Nextint                 28
  // [ nested_field
  // 1: b_field       1         29
  // 3+4: f_field     8         36
  // 1: o_field       (9)       37
  // nested_field ]
  // 3+4: Emheader              44
  // 4: Nextint                 48
  // 4+6: str_field             58
  // 2+4: Emheader              64
  // 8: ull_field               (72)
  const size_t arr[] = {72, 2, 4, 9, 10, 8};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableAppendableStruct)
{
  MutableAppendableStruct mas;
  init(mas);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableAppendableStruct_xtag>(value_writer, mas);

  // Layout           Nested    Total size
  // 4: Dheader                 4
  // 4: Emheader                8
  // 2: s_field                 10
  // 2+4: Emheader              16
  // 4: l_field                 20
  // 4: Emheader                24
  // 4: Nextint                 28
  // [ nested_field
  // 4: Dheader       4         32
  // 1: b_field       5         33
  // 3+4: f_field     12        40
  // 1: o_field       (13)      41
  // nested_field ]
  // 3+4: Emheader              48
  // 4: Nextint                 52
  // 4+6: str_field             62
  // 2+4: Emheader              68
  // 8: ull_field               (76)
  const size_t arr[] = {76, 2, 4, 13, 10, 8};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableMutableStruct)
{
  MutableMutableStruct mms;
  init(mms);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableMutableStruct_xtag>(value_writer, mms);

  // Layout           Nested    Total size
  // 4: Dheader                 4
  // 4: Emheader                8
  // 2: s_field                 10
  // 2+4: Emheader              16
  // 4: l_field                 20
  // 4: Emheader                24
  // 4: Nextint                 28
  // [ nested_field
  // 4: Dheader       4         32
  // 4: Emheader      8         36
  // 1: b_field       9         37
  // 3+4: Emheader    16        44
  // 4: f_field       20        48
  // 4: Emheader      24        52
  // 1: o_field       (25)      53
  // nested_field ]
  // 3+4: Emheader              60
  // 4: Nextint                 64
  // 4+6: str_field             74
  // 2+4: Emheader              80
  // 8: ull_field               (88)
  const size_t arr[] = {88, 2, 4, 25, 1, 4, 1, 10, 8};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

// TODO(sonndinh): Check component sizes

void init(FinalUnion& fu)
{
  AppendableMutableStruct ams;
  init(ams);
  fu.nested_field(ams);
}

void init(AppendableUnion& au)
{
  MutableFinalStruct mfs;
  init(mfs);
  au.nested_field(mfs);
}

void init(MutableUnion& mu)
{
  FinalAppendableStruct fas;
  init(fas);
  mu.nested_field(fas);
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalUnion)
{
  FinalUnion fu;
  init(fu);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalUnion_xtag>(value_writer, fu);
}


TEST(dds_DCPS_SerializedSizeValueWriter, AppendableUnion)
{
  AppendableUnion au;
  init(au);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableUnion_xtag>(value_writer, au);
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableUnion)
{
  MutableUnion mu;
  init(mu);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableUnion_xtag>(value_writer, mu);
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalComplexStruct)
{
  FinalComplexStruct fcs;
  fcs.c_field = 'd';

  // FinalUnion fu;
  // init(fu);
  // fcs.nested_union = fu;
  init(fcs.nested_union);

  fcs.ll_field = 123;
  init_base_struct(fcs.nested_struct);
  init(fcs.nnested_struct);
  fcs.str_field = "my string";

  fcs.seq_field.length(2);
  fcs.seq_field[0] = 0;
  fcs.seq_field[1] = 1;

  fcs.arr_field[0] = 10;
  fcs.arr_field[1] = 20;

  fcs.md_arr_field[0][0] = 0;
  fcs.md_arr_field[0][1] = 1;
  fcs.md_arr_field[1][0] = 10;
  fcs.md_arr_field[1][1] = 11;

  fcs.nested_seq.length(2);
  AppendableStruct as;
  init_base_struct(as);
  fcs.nested_seq[0] = as;
  fcs.nested_seq[1] = as;

  MutableStruct ms;
  init_base_struct(ms);
  fcs.nested_arr[0] = ms;
  fcs.nested_arr[1] = ms;

  fcs.f_field = 2.0f;

  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalComplexStruct_xtag>(value_writer, fcs);

  // Layout                   Running size     Component sizes
  // 1: c_field                    1            (276)
  // [= nested_union
  // 3+4: disc                     8
  // [== nested_field                           (60)
  // 4: DHEADER                    12
  // 2: s_field                    14
  // 2+4: l_field                  20
  // [=== nested_field                          (25)
  // 4: DHEADER                    24
  // 4: EMHEADER                   28
  // 1: b_field                    29           (1)
  // 3+4: EMHEADER                 36
  // 4: f_field                    40           (4)
  // 4: EMHEADER                   44
  // 1: o_field                    45           (1)
  // nested_field ===]
  // 3+4+6: str_field              58
  // 2+8: ull_field                68
  // nested_field ==]
  // nested_union =]
  // 8: ll_field                   76
  // [= nested_struct                           (13)
  // 4: DHEADER                    80
  // 1: b_field                    81
  // 3+4: f_field                  88
  // 1: o_field                    89
  // nested_struct =]
  // [= nnested_struct
  // 1+2: s_field                  92
  // 4: l_field                    96
  // [== nested_field                           (13)
  // 4: DHEADER                    100
  // 1: b_field                    101
  // 3+4: f_field                  108
  // 1: o_field                    109
  // nested_field =]
  // 3+4+6: str_field              122
  // 2+8: ull_field                132
  // nnested_struct =]
  // 4+10: str_field               146
  // 2+4+2+2: seq_field            156
  // 4+4: arr_field                164
  // 2+2+2+2: md_arr_field         172
  // [= nested_seq                              (37)
  // 4: DHEADER                    176
  // 4: length                     180
  // [== nested_seq[0]                          (13)
  // 4: DHEADER                    184
  // 1+3+4+1: fields               193
  // nested_seq[0] ==]
  // [== nested_seq[1]                          (13)
  // 3+4: DHEADER                  200
  // 1+3+4+1: fields               209
  // nested_seq[1] ==]
  // nested_seq =]
  // [= nested_arr                              (57)
  // 3+4: DHEADER                  216
  // [== nested_arr[0]                          (25)
  // 4: DHEADER                    220
  // 4+1+3+4+4+4+1=21: fields      241          (1) (4) (1)
  // nested_arr[0] =]
  // [== nested_arr[1]                          (25)
  // 3+4: DHEADER                  248
  // 4+1+3+4+4+4+1=21: fields      269          (1) (4) (1)
  // nested_arr[1] ==]
  // nested_arr =]
  // 3+4: f_field                  276
  const size_t arr[] = {276, 60, 25, 1, 4, 1, 13, 13, 37, 13, 13, 57, 25, 1, 4, 1, 25, 1, 4, 1};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableComplexStruct)
{
  AppendableComplexStruct acs;
  acs.s_field = 1;

  init(acs.nested_union);

  acs.f_field = 1.0f;
  init_base_struct(acs.nested_struct);
  init(acs.nnested_struct);
  acs.str_field = "my string";

  acs.seq_field.length(2);
  acs.seq_field[0] = 0;
  acs.seq_field[1] = 1;

  acs.arr_field[0] = ONE;
  acs.arr_field[1] = TWO;

  acs.md_arr_field[0][0] = 0;
  acs.md_arr_field[0][1] = 1;
  acs.md_arr_field[1][0] = 10;
  acs.md_arr_field[1][1] = 11;

  acs.nested_seq.length(2);
  MutableStruct ms;
  init_base_struct(ms);
  acs.nested_seq[0] = ms;
  acs.nested_seq[1] = ms;

  FinalStruct fs;
  init_base_struct(fs);
  acs.nested_arr[0] = fs;
  acs.nested_arr[1] = fs;

  acs.d_field = 1.0;

  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableComplexStruct_xtag>(value_writer, acs);

  // Layout              Running size   Component sizes
  // 4: DHEADER                          (332)
  // 2: s_field                    2
  // [= nested_union                     (80)
  // 2+4: DHEADER                  8
  // 4: disc                       12
  // [== nested_field                    (72)
  // 4: DHEADER                    16
  // 4: EMHEADER                   20
  // 2: s_field                    22    (2)
  // 2+4: EMHEADER                 28
  // 4: l_field                    32    (4)
  // 4: EMHEADER                   36
  // 4: NEXTINT                    40
  // [=== nested_field                   (9)
  // 1: b_field                    41
  // 3+4: f_field                  48
  // 1: o_field                    49
  // nested_field ===]
  // 3+4: EMHEADER                 56
  // 4: NEXTINT                    60
  // 4+6: str_field                70    (10)
  // 2+4: EMHEADER                 76
  // 8: ull_field                  84    (8)
  // nested_field ==]
  // nested_union =]
  // 4: f_field                    88
  // [= nested_struct                    (25)
  // 4: DHEADER                    92
  // 4: EMHEADER                   96
  // 1: b_field                    97    (1)
  // 3+4: EMHEADER                 104
  // 4: f_field                    108   (4)
  // 4: EMHEADER                   112
  // 1: o_field                    113   (1)
  // nested_struct =]
  // [= nnested_struct                   (60)
  // 3+4: DHEADER                  120
  // 2: s_field                    122
  // 2+4: l_field                  128
  // [== nested_field                    (25)
  // 4: DHEADER                    132
  // 4: EMHEADER                   136
  // 1: b_field                    137   (1)
  // 3+4: EMHEADER                 144
  // 4: f_field                    148   (4)
  // 4: EMHEADER                   152
  // 1: o_field                    153   (1)
  // nested_field ==]
  // 3+4+6: str_field              166
  // 2+8: ull_field                176
  // nnested_struct =]
  // 4+10: str_field               190
  // 2+4+4+4: seq_field            204
  // [= arr_field
  // 4: DHEADER                    208   (12)
  // 4+4: elements                 216
  // arr_field =]
  // 4+4+4+4: md_arr_field         232
  // [= nested_seq                       (61)
  // 4: DHEADER                    236
  // 4: length                     240
  // [== nested_seq[0]
  // 4: DHEADER                    244   (25)
  // 4: EMHEADER                   248
  // 1: b_field                    249   (1)
  // 3+4: EMHEADER                 256
  // 4: f_field                    260   (4)
  // 4: EMHEADER                   264
  // 1: o_field                    265   (1)
  // nested_seq[0] ==]
  // [== nested_seq[1]
  // 3+4: DHEADER                  272   (25)
  // 4: EMHEADER                   276
  // 1: b_field                    277   (1)
  // 3+4: EMHEADER                 284
  // 4: f_field                    288   (4)
  // 4: EMHEADER                   292
  // 1: o_field                    293   (1)
  // nested_seq[1] ==]
  // nested_seq =]
  // [= nested_arr
  // 3+4: DHEADER                  300   (21)
  // [== nested_arr[0]
  // 1: b_field                    301
  // 3+4: f_field                  308
  // 1: o_field                    309
  // nested_arr[0] ==]
  // [== nested_arr[1]
  // 1: b_field                    310
  // 2+4: f_field                  316
  // 1: o_field                    317
  // nested_arr[1] ==]
  // nested_arr =]
  // 3+8: d_field                  328
  const size_t arr[] = {332,80,72,2,4,9,10,8,25,1,4,1,60,25,1,4,1,12,61,25,1,4,1,25,1,4,1,21};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableComplexStruct)
{
  MutableComplexStruct mcs;
}
