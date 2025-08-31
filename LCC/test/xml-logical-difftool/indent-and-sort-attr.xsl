<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!--
  xmlns="http://www.w3.org/1999/xhtml"
  xmlns:xh="http://www.w3.org/1999/xhtml"
  exclude-result-prefixes="xh">
-->

<xsl:output
  method="xml"
  encoding="UTF-8"
  indent="yes"/>

<!--
this is dangerous for elements which
only have whitespace like e.g.
<p> <b>hello</b><p> will get <p><b>hello</b><p>
use preserve-whitespace for this
special elements which can contain whitespace
--> 
<xsl:strip-space elements="*"/>
<!-- <xsl:preserve-space elements="xh:p xh:span xh:li xh:td xh:a"/> -->

<!-- copy all and sort attributes -->
<xsl:template match="@*|node()">
  <xsl:copy>
    <xsl:apply-templates select="@*">
        <xsl:sort select="name()"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="node()"/>
  </xsl:copy>
</xsl:template>

</xsl:stylesheet>