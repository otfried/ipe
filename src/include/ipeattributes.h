// -*- C++ -*-
// --------------------------------------------------------------------
// Ipe object attributes
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2023 Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef IPEATTRIBUTES_H
#define IPEATTRIBUTES_H

#include "ipebase.h"
#include "ipegeo.h"

// --------------------------------------------------------------------

namespace ipe {

  /*! \brief The different kinds of attributes.
    \ingroup attr

    The same symbolic attribute (say "normal") has a different value
    in the StyleSheet depending on the Kind of attribute. The main use
    for Kind is as an argument to StyleSheet::find.

    ESymbol, EGradient, ETiling, and EEffect have their own lookup
    methods in the StyleSheet.  The values are still useful as an
    argument to allNames(), has(), and findDefinition().
  */
  enum Kind { EPen = 0, ESymbolSize, EArrowSize, EColor,
	      EDashStyle, ETextSize, ETextStretch,
	      ETextStyle, ELabelStyle,
	      EGridSize, EAngleSize, EOpacity, ETiling,
	      ESymbol, EGradient, EEffect };

  /*! \ingroup attr */
  extern const char * const kind_names[];

  /*! \brief A Property identifies an attribute that an object can have.
    \ingroup attr

    The Property identifies a unique attribute of an object, while
    different Property values can be of the same ipe::Kind.  For
    instance, both EPropStrokeColor and EPropFillColor identify an
    Attribute of Kind EColor.
  */
  enum Property { EPropPen = 0, EPropSymbolSize,
		  EPropFArrow, EPropRArrow,
		  EPropFArrowSize, EPropRArrowSize,
		  EPropFArrowShape, EPropRArrowShape,
		  EPropStrokeColor, EPropFillColor, EPropMarkShape,
		  EPropPathMode, EPropDashStyle,
		  EPropTextSize, EPropTextStyle, EPropLabelStyle,
		  EPropOpacity, EPropStrokeOpacity, EPropTiling, EPropGradient,
		  EPropHorizontalAlignment, EPropVerticalAlignment,
		  EPropLineJoin, EPropLineCap, EPropFillRule,
		  EPropPinned, EPropTransformations,
		  EPropTransformableText, EPropSplineType,
		  EPropMinipage, EPropWidth,
		  EPropDecoration,
  };

  /*! \ingroup attr */
  extern const char * const property_names[];

  //! Path mode (stroked, filled, or both).
  /*! \ingroup attr */
  enum TPathMode { EStrokedOnly, EStrokedAndFilled, EFilledOnly };

  //! Horizontal alignment.
  /*! \ingroup attr */
  enum THorizontalAlignment { EAlignLeft, EAlignRight, EAlignHCenter };

  //! Vertical alignment.
  /*! \ingroup attr */
  enum TVerticalAlignment { EAlignBottom, EAlignBaseline,
      EAlignTop, EAlignVCenter };

  //! Spline type.
  /*! \ingroup attr */
  enum TSplineType { EBSpline, ECardinalSpline, ESpiroSpline };

  //! Line join style.
  /*! \ingroup attr */
  /*! The EDefaultJoin means to use the setting from the style sheet. */
  enum TLineJoin { EDefaultJoin, EMiterJoin, ERoundJoin, EBevelJoin };

  //! Line cap style.
  /*! \ingroup attr */
  /*! The EDefaultCap means to use the setting from the style sheet. */
  enum TLineCap { EDefaultCap, EButtCap, ERoundCap, ESquareCap };

  //! Fill rule.
  /*! \ingroup attr */
  /*! The EDefaultRule means to use the setting from the style sheet. */
  enum TFillRule { EDefaultRule, EWindRule, EEvenOddRule };

  //! Pinning status of objects.
  /*! \ingroup attr */
  enum TPinned { ENoPin = 0x00, EHorizontalPin = 0x01,
      EVerticalPin = 0x02, EFixedPin = 0x03 };

  //! Transformations that are permitted for an object.
  /*! \ingroup attr */
  enum TTransformations { ETransformationsTranslations,
      ETransformationsRigidMotions,
      ETransformationsAffine };

  //! Selection status of an object on the page
  /*! \ingroup attr */
  enum TSelect { ENotSelected = 0, EPrimarySelected,
      ESecondarySelected };

  // --------------------------------------------------------------------

  class Color {
  public:
    //! Default constructor.
    Color() { /* nothing */ }
    explicit Color(String str);
    explicit Color(int r, int g, int b);
    void save(Stream &stream) const;
    void saveRGB(Stream &stream) const;
    bool isGray() const;
    bool operator==(const Color &rhs) const;
    inline bool operator!=(const Color &rhs) const {return !(*this == rhs); }
  public:
    Fixed iRed, iGreen, iBlue;
  };

  //! A tiling pattern.
  /*! \ingroup attr */
  struct Tiling {
    Angle iAngle;
    double iStep;
    double iWidth;
  };

  //! A gradient pattern.
  /*! \ingroup attr */
  struct Gradient {
    //! A color stop.
    struct Stop {
      //! Offset between 0.0 and 1.0.
      double offset;
      //! The color at this offset.
      Color color;
    };
    //! There are two types of gradients, along an axis or between two circles.
    enum TType { EAxial = 2, ERadial = 3 };
    //! The type of gradient: axial or radial.
    TType iType;
    //! The coordinates of the axis endpoints, or the two circle centers.
    Vector iV[2];
    //! The radii of the two circles (not used for axial gradients).
    double iRadius[2];
    //! Whether to extend the gradient beyond the endpoints.
    bool iExtend;
    //! Gradient transformation.
    Matrix iMatrix;
    //! The color stops.
    std::vector<Stop> iStops;
  };

  //! Layout of a Page.
  /*! \ingroup attr */
  struct Layout {
    //! Create null layout.
    Layout() { iPaperSize.x = -1.0; }
    //! Is this an undefined (null) layout?
    bool isNull() const { return iPaperSize.x < 0.0; }
    //! Return rectangle describing the paper.
    Rect paper() const { return Rect(-iOrigin, iPaperSize - iOrigin); }
    //! Dimensions of the media.
    Vector iPaperSize;
    //! Origin of the Ipe coordinate system relative to the paper.
    Vector iOrigin;
    //! Size of the frame (the drawing area).
    Vector iFrameSize;
    //! Paragraph skip (between textboxes).
    double iParagraphSkip;
    //! Crop paper to drawing.
    bool iCrop;
  };

  //! Padding for text bounding box.
  /*! \ingroup attr */
  struct TextPadding {
  public:
    double iLeft, iRight, iTop, iBottom;
  };

  struct Effect {
  public:
    //! The various fancy effects that Acrobat Reader will show.
    enum TEffect {
      ENormal, ESplitHI, ESplitHO, ESplitVI, ESplitVO,
      EBlindsH, EBlindsV, EBoxI, EBoxO,
      EWipeLR, EWipeBT, EWipeRL, EWipeTB,
      EDissolve, EGlitterLR, EGlitterTB, EGlitterD,
      EFlyILR, EFlyOLR, EFlyITB, EFlyOTB, EPushLR, EPushTB,
      ECoverLR, ECoverLB, EUncoverLR, EUncoverTB,
      EFade
    };

    Effect();
    void pageDictionary(Stream &stream) const;

  public:
    TEffect iEffect;
    int iTransitionTime;
    int iDuration;
  };

  // --------------------------------------------------------------------

  class Repository {
  public:
    static Repository *get();
    static void cleanup();
    String toString(int index) const;
    int toIndex(String str);
    // int getIndex(String str) const;
  private:
    Repository();
    static Repository *singleton;
    std::vector<String> iStrings;
  };

  // --------------------------------------------------------------------

  class Attribute {
    enum { EMiniMask = 0xc0000000, ETypeMask = 0xe0000000,
	   ESymbolic = 0x80000000, EFixed = 0x40000000,
	   EAbsolute = 0xc0000000, EEnum = 0xe0000000,
	   EFixedMask = 0x3fffffff, ENameMask = 0x1fffffff,
	   EWhiteValue = ((1000 << 20) + (1000 << 10) + 1000),
	   EOneValue = EFixed|1000 };

  public:
    //! Default constructor.
    explicit Attribute() { /* nothing */ }

    explicit Attribute(bool symbolic, String name);
    explicit Attribute(Fixed value);
    explicit Attribute(Color color);
    static Attribute Boolean(bool flag) { return Attribute(EEnum + flag); }
    explicit Attribute(THorizontalAlignment align) { iName = EEnum + int(align) +2; }
    explicit Attribute(TVerticalAlignment align) { iName = EEnum + int(align) + 5; }
    explicit Attribute(TLineJoin join) { iName = EEnum + int(join) + 9; }
    explicit Attribute(TLineCap cap) { iName = EEnum + int(cap) + 13; }
    explicit Attribute(TFillRule rule) { iName = EEnum + int(rule) + 17; }
    explicit Attribute(TPinned pin) { iName = EEnum + int(pin) + 20; }
    explicit Attribute(TTransformations trans) { iName = EEnum + int(trans) + 24; }
    explicit Attribute(TPathMode pm) { iName = EEnum + int(pm) + 27; }
    explicit Attribute(TSplineType st) { iName = EEnum + int(st) + 30; }

    //! Is it symbolic?
    inline bool isSymbolic() const {
      return ((iName & ETypeMask) == ESymbolic); }
    //! Is it an absolute string value?
    inline bool isString() const {
      return ((iName & ETypeMask) == EAbsolute); }
    //! Is it a color?
    inline bool isColor() const {
      return ((iName & EMiniMask)  == 0); }
    //! Is it a number?
    inline bool isNumber() const {
      return ((iName & EMiniMask) == EFixed); }
    //! Is it an enumeration?
    inline bool isEnum() const {
      return ((iName & ETypeMask) == EEnum); }

    //! Is it a boolean?
    inline bool isBoolean() const {
      return (isEnum() && 0 <= index() && index() <= 1); }

    //! Is it the symbolic name "normal"?
    inline bool isNormal() const { return (iName == ESymbolic); }

    //! Return index into Repository.
    inline int index() const { return iName & ENameMask; }

    int internal() const { return iName; }

    String string() const;
    Fixed number() const;
    Color color() const;

    bool boolean() const { return bool(index()); }
    THorizontalAlignment horizontalAlignment() const {
      return THorizontalAlignment(index() - 2); }
    TVerticalAlignment verticalAlignment() const {
      return TVerticalAlignment(index() - 5); }
    TLineJoin lineJoin() const {return TLineJoin(index() - 9); }
    TLineCap lineCap() const { return TLineCap(index() - 13); }
    TFillRule fillRule() const { return TFillRule(index() - 17); }
    TPinned pinned() const { return TPinned(index() - 20); }
    TTransformations transformations() const {
      return TTransformations(index() - 24); }
    TPathMode pathMode() const { return TPathMode(index() - 27); }
    TSplineType splineType() const { return TSplineType(index() - 30); }

    //! Are two values equal (only compares index!)
    inline bool operator==(const Attribute &rhs) const {
      return iName == rhs.iName; }

    //! Are two values different (only compares index!)
    inline bool operator!=(const Attribute &rhs) const {
      return iName != rhs.iName; }

    //! Create absolute black color.
    inline static Attribute BLACK() { return Attribute(0); }
    //! Create absolute white color.
    inline static Attribute WHITE() { return Attribute(EWhiteValue); }
    //! Create absolute number one.
    inline static Attribute ONE() { return Attribute(EOneValue); }

    //! Create symbolic attribute "normal".
    inline static Attribute NORMAL() { return Attribute(ESymbolic); }
    //! Create symbolic attribute "undefined"
    inline static Attribute UNDEFINED() { return Attribute(ESymbolic + 1); }
    //! Create symbolic attribute "Background"
    inline static Attribute BACKGROUND() { return Attribute(ESymbolic + 2); }
    //! Create symbolic attribute "sym-stroke"
    inline static Attribute SYM_STROKE() { return Attribute(ESymbolic + 3); }
    //! Create symbolic attribute "sym-fill"
    inline static Attribute SYM_FILL() { return Attribute(ESymbolic + 4); }
    //! Create symbolic attribute "sym-pen"
    inline static Attribute SYM_PEN() { return Attribute(ESymbolic + 5); }
    //! Create symbolic attribute "arrow/normal(spx)"
    inline static Attribute ARROW_NORMAL() { return Attribute(ESymbolic + 6); }
    //! Create symbolic attribute "opaque"
    inline static Attribute OPAQUE() { return Attribute(ESymbolic + 7); }
    //! Create symbolic attribute "arrow/arc(spx)"
    inline static Attribute ARROW_ARC() { return Attribute(ESymbolic + 8); }
    //! Create symbolic attribute "arrow/farc(spx)"
    inline static Attribute ARROW_FARC() { return Attribute(ESymbolic + 9); }
    //! Create symbolic attribute "arrow/ptarc(spx)"
    inline static Attribute ARROW_PTARC() { return Attribute(ESymbolic + 10); }
    //! Create symbolic attribute "arrow/fptarc(spx)"
    inline static Attribute ARROW_FPTARC() { return Attribute(ESymbolic + 11); }

    //! Is it one of the symbolic attributes "arrow/*arc(spc)"?
    inline bool isArcArrow() const {
      return ESymbolic + 8 <= iName && iName <= ESymbolic + 11;
    }

    bool isMidArrow() const;

    static Attribute makeColor(String str, Attribute deflt);
    static Attribute makeScalar(String str, Attribute deflt);
    static Attribute makeDashStyle(String str);
    static Attribute makeTextSize(String str);

    static Attribute normal(Kind kind);

  private:
    inline Attribute(int index) : iName(index) { /* nothing */ }
    explicit Attribute(bool symbolic, int index);
  private:
    uint32_t iName;

    friend class StyleSheet;
  };

  /*! \var AttributeSeq
    \ingroup attr
    \brief A sequence of attribute values.
  */
  typedef std::vector<Attribute> AttributeSeq;

  // --------------------------------------------------------------------

  /*! \var AttributeMapping
    \ingroup attr
    \brief Mapping one symbolic attribute to another one
  */
  struct AttributeMapping {
    Kind kind;
    Attribute from;
    Attribute to;
  };

  class AttributeMap {
  public:
    int count() const noexcept { return iMap.size(); }
    Attribute map(Kind kind, Attribute sym) const;
    void saveAsXml(Stream &stream) const;
    void add(const AttributeMapping &map);
  public:
    std::vector<AttributeMapping> iMap;
  };

  // --------------------------------------------------------------------

  class AllAttributes {
  public:
    AllAttributes();
    TPathMode iPathMode;        //!< Should we stroke and/or fill?
    Attribute iStroke;          //!< Stroke color.
    Attribute iFill;            //!< Fill color.
    Attribute iDashStyle;       //!< Dash style.
    Attribute iPen;             //!< Pen (that is, line width).
    bool iFArrow;               //!< Arrow forward?
    bool iRArrow;               //!< Reverse arrow?
    Attribute iFArrowShape;     //!< Shape of forward arrows
    Attribute iRArrowShape;     //!< Shape of reverse arrows
    Attribute iFArrowSize;      //!< Forward arrow size.
    Attribute iRArrowSize;      //!< Reverse arrow size.
    Attribute iSymbolSize;      //!< Symbol size.
    Attribute iTextSize;        //!< Text size.
    //! Horizontal alignment of label objects.
    THorizontalAlignment iHorizontalAlignment;
    //! Vertical alignment of label objects.
    TVerticalAlignment iVerticalAlignment;
    Attribute iTextStyle;       //!< Text style for minipages.
    Attribute iLabelStyle;      //!< Text style for labels.
    TPinned iPinned;            //!< Pinned status.
    //! Should newly created text be transformable?
    /*! If this is false, newly created text will only allow
      translations.  Otherwise, the value of iTranslations is used (as
      for other objects). */
    bool iTransformableText;
    /*! What kind of splines should be created? */
    TSplineType iSplineType;
    //! Allowed transformations.
    TTransformations iTransformations;
    TLineJoin iLineJoin;        //!< Line join style.
    TLineCap iLineCap;          //!< Line cap style.
    TFillRule iFillRule;        //!< Shape fill rule.
    Attribute iOpacity;         //!< Opacity.
    Attribute iStrokeOpacity;   //!< Stroke opacity.
    Attribute iTiling;          //!< Tiling pattern.
    Attribute iGradient;        //!< Gradient pattern.
    Attribute iMarkShape;       //!< Shape of Mark to create.
  };

  // --------------------------------------------------------------------

  /*! \relates Color */
  inline Stream &operator<<(Stream &stream, const Color &attr)
  {
    attr.save(stream); return stream;
  }

} // namespace

// --------------------------------------------------------------------
#endif
