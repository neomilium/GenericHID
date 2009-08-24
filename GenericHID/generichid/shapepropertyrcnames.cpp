#include "stdafx.h"
#include "shapepropertyrcnames.h"

ShapePropertyRCNames::ShapePropertyRCNames( const QString &sName, const QString &sDescription )
: ShapePropertyString( sName, sDescription )
{
}

ShapePropertyRCNames::~ShapePropertyRCNames(void)
{
}

QtProperty *ShapePropertyRCNames::getQtProperty()
{
    QtProperty *pProp = m_keyMatrixNameManager->addProperty( m_sName );
    pProp->setToolTip( m_sDescription );
    pProp->setWhatsThis( m_sDescription );
    return pProp;
}


void ShapePropertyRCNames::setValue(QtProperty *qtProp, PropertyValue *propVal) const
{
    PropertyValueString *val = dynamic_cast<PropertyValueString *>( propVal );
    if ( val != NULL )
	m_keyMatrixNameManager->setValue( qtProp, val->Value );
}

void ShapePropertyRCNames::getValue(QtProperty *qtProp, PropertyValue *propVal) const
{
    PropertyValueString *val = dynamic_cast<PropertyValueString *>( propVal );
    if ( val != NULL )
	val->Value = m_keyMatrixNameManager->value( qtProp );
}

